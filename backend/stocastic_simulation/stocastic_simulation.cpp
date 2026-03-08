#include "stocastic_simulation.hpp"
#include "database_manager.hpp"
#include "datatypes.hpp"
#include <immintrin.h>

#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdio>

#include <omp.h>

// ── Static member definitions ─────────────────────────────────────────
std::unordered_map<std::string, SimulationResult> StochasticSimulator::sim_cache;
std::shared_mutex StochasticSimulator::cache_mutex;

// ── Cache helpers ─────────────────────────────────────────────────────

void StochasticSimulator::evict_expired_cache() {
    auto now = std::chrono::system_clock::now();
    std::unique_lock lock(cache_mutex);
    for (auto it = sim_cache.begin(); it != sim_cache.end(); ) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.created_at).count();
        if (age > CACHE_TTL_SECONDS)
            it = sim_cache.erase(it);
        else
            ++it;
    }
}

void StochasticSimulator::clear_cache() {
    std::unique_lock lock(cache_mutex);
    sim_cache.clear();
}

// Returns reference to cached paths, running simulation on miss.
// Caller must not hold cache_mutex.
const std::vector<std::vector<double>>&
StochasticSimulator::get_or_run_simulation(size_t SSN, Date end_date) {
    std::string key = make_cache_key(SSN, end_date);

    // Fast path — shared read lock
    {
        std::shared_lock lock(cache_mutex);
        auto it = sim_cache.find(key);
        if (it != sim_cache.end())
            return it->second.paths;
    }

    // Slow path — run simulation, then write
    auto paths = stochastic_analysis(SSN, end_date);

    std::unique_lock lock(cache_mutex);
    // Check again after acquiring write lock (another thread may have beaten us)
    auto it = sim_cache.find(key);
    if (it != sim_cache.end())
        return it->second.paths;

    sim_cache[key] = SimulationResult{
        .paths      = std::move(paths),
        .created_at = std::chrono::system_clock::now()
    };
    return sim_cache[key].paths;
}

// ── predict_variation (stub — replace with real model) ───────────────

std::pair<double, double> StochasticSimulator::predict_variation(MC& m) {
    return {0.0, 1.0};
}

// ── stochastic_analysis ───────────────────────────────────────────────

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, Date pay_date) {
    size_t job      = 1;
    size_t location = 1;
    try {
        job      = DataBaseManager::get_current_job(SSN);
        location = DataBaseManager::get_current_location(SSN);
    } catch (...) {}
    return stochastic_analysis(SSN, job, location, pay_date);
}

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, size_t ocupation, size_t location, Date pay_date) {
    auto now      = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);

    Date curr_date{local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    int m = pay_date - curr_date;

    std::cout << "  [sim] SSN=" << SSN << " days=" << m << " paths=" << N_SIMULATIONS << "\n";

    if (m <= 0) {
        std::cout << "  [sim] pay_date in the past, returning empty\n";
        return {};
    }

    auto return_value = std::vector<std::vector<double>>(N_SIMULATIONS, std::vector<double>(m, 0.0));
    auto pay_days     = DataBaseManager::get_pay_days({1, SSN, location, ocupation});

    auto t1 = std::chrono::high_resolution_clock::now();

    std::vector<DayCache> cache(m);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < m; i++) {
        Date target = curr_date + Date{i, 0, 0};
        auto data = MC{
            .day           = target.day,
            .month         = target.month,
            .year          = target.year,
            .day_of_week   = day_of_week(target),
            .location_id   = location,
            .profession_id = ocupation
        };
        auto [mean, sd] = predict_variation(data);
        double payday_injection = 0.0;
        auto it = pay_days->find(target);
        if (it != pay_days->end()) payday_injection = it->second;
        cache[i] = {mean, sd, payday_injection};
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "  [sim] Step 1 (prefetch) done in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms\n";

    #pragma omp parallel
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count()
                        ^ (std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::default_random_engine generator(seed);
        std::normal_distribution<double> normal(0.0, 1.0);

        #pragma omp for schedule(static)
        for (int i = 0; i < (int)N_SIMULATIONS; i++) {
            for (int j = 0; j < m; j++) {
                return_value[i][j] = normal(generator) * cache[j].sd
                                   + cache[j].mean
                                   + cache[j].payday;
            }
        }
    }

    auto t3 = std::chrono::high_resolution_clock::now();
    std::cout << "  [sim] Step 2 (simulation) done in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << "ms\n";
    std::cout << "  [sim] Total: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t1).count() << "ms\n";

    return return_value;
}

// ── Cumulative balance helper ──────────────────────────────────────────
// Converts daily deltas → prefix sums in place, returns new matrix.
static std::vector<std::vector<double>> to_cumulative(const std::vector<std::vector<double>>& paths) {
    std::vector<std::vector<double>> cum(paths.size(), std::vector<double>(paths[0].size(), 0.0));
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        double running = 0.0;
        for (int j = 0; j < (int)paths[i].size(); j++) {
            running += paths[i][j];
            cum[i][j] = running;
        }
    }
    return cum;
}

// Cumulative balance at each milestone day for every path
static std::vector<std::vector<double>> compute_cumulative_at_milestones(
    const std::vector<std::vector<double>>& paths,
    const std::vector<std::pair<Date, double>>& milestones,
    Date start_date
) {
    std::vector<std::vector<double>> cumulative(paths.size(), std::vector<double>(milestones.size(), 0.0));

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        double balance  = 0.0;
        size_t m_idx    = 0;
        int    next_day = milestones[0].first - start_date;

        for (int j = 0; j < (int)paths[i].size() && m_idx < milestones.size(); j++) {
            balance += paths[i][j];
            if (j == next_day) {
                cumulative[i][m_idx] = balance;
                m_idx++;
                if (m_idx < milestones.size())
                    next_day = milestones[m_idx].first - start_date;
            }
        }
    }
    return cumulative;
}

// ── Internal _from_paths helpers ──────────────────────────────────────

double StochasticSimulator::_odds_from_paths(
    const std::vector<std::vector<double>>& paths,
    double amount, double interest_rate,
    const std::vector<std::pair<Date, double>>& milestones,
    Date start_date
) {
    auto cumulative = compute_cumulative_at_milestones(paths, milestones, start_date);
    int successes = 0;
    #pragma omp parallel for reduction(+:successes) schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        bool ok = true;
        for (int m = 0; m < (int)milestones.size(); m++) {
            if (cumulative[i][m] < milestones[m].second) { ok = false; break; }
        }
        if (ok) successes++;
    }
    return (100.0 * successes) / (double)paths.size();
}

double StochasticSimulator::_profit_from_paths(
    const std::vector<std::vector<double>>& paths,
    double amount, double interest_rate,
    const std::vector<std::pair<Date, double>>& milestones,
    Date start_date
) {
    auto cumulative = compute_cumulative_at_milestones(paths, milestones, start_date);
    int successes = 0;
    #pragma omp parallel for reduction(+:successes) schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        bool ok = true;
        for (int m = 0; m < (int)milestones.size(); m++) {
            if (cumulative[i][m] < milestones[m].second) { ok = false; break; }
        }
        if (ok) successes++;
    }
    int failures = (int)paths.size() - successes;
    return (successes * amount * interest_rate) + (failures * (-amount));
}

double StochasticSimulator::_best_rate_from_paths(
    const std::vector<std::vector<double>>& paths,
    double amount,
    const std::vector<std::pair<Date, double>>& milestones,
    Date start_date
) {
    auto cumulative  = compute_cumulative_at_milestones(paths, milestones, start_date);
    int  n_paths     = (int)paths.size();
    int  n_m         = (int)milestones.size();
    double best_rate = 0.0, best_profit = std::numeric_limits<double>::lowest();

    for (double rate = 0.0; rate <= 1.0; rate += 0.005) {
        double per_payment = (amount * (1.0 + rate)) / n_m;
        int successes = 0;
        #pragma omp parallel for reduction(+:successes) schedule(static)
        for (int i = 0; i < n_paths; i++) {
            bool ok = true;
            for (int m = 0; m < n_m; m++) {
                if (cumulative[i][m] < per_payment * (m + 1)) { ok = false; break; }
            }
            if (ok) successes++;
        }
        double profit = (successes * amount * rate) + ((n_paths - successes) * (-amount));
        if (profit > best_profit) { best_profit = profit; best_rate = rate; }
    }
    return best_rate;
}

double StochasticSimulator::_min_rate_from_paths(
    const std::vector<std::vector<double>>& paths,
    double amount,
    std::vector<double> candidate_rates,
    const std::vector<std::pair<Date, double>>& milestones,
    Date start_date
) {
    auto cumulative = compute_cumulative_at_milestones(paths, milestones, start_date);
    int  n_paths    = (int)paths.size();
    int  n_m        = (int)milestones.size();

    for (double rate : candidate_rates) {
        double per_payment = (amount * (1.0 + rate)) / n_m;
        int successes = 0;
        #pragma omp parallel for reduction(+:successes) schedule(static)
        for (int i = 0; i < n_paths; i++) {
            bool ok = true;
            for (int m = 0; m < n_m; m++) {
                if (cumulative[i][m] < per_payment * (m + 1)) { ok = false; break; }
            }
            if (ok) successes++;
        }
        double profit = (successes * amount * rate) + ((n_paths - successes) * (-amount));
        if (profit > 0.0) return rate;
    }
    return -1.0;
}

std::vector<double> StochasticSimulator::_schedule_odds_from_paths(
    const std::vector<std::vector<double>>& paths,
    const std::vector<std::pair<Date, double>>& milestones,
    Date start_date
) {
    auto cumulative = compute_cumulative_at_milestones(paths, milestones, start_date);
    std::vector<double> odds(milestones.size(), 0.0);

    for (int m = 0; m < (int)milestones.size(); m++) {
        double required = milestones[m].second;
        int successes   = 0;
        #pragma omp parallel for reduction(+:successes) schedule(static)
        for (int i = 0; i < (int)paths.size(); i++) {
            if (cumulative[i][m] >= required) successes++;
        }
        odds[m] = (100.0 * successes) / (double)paths.size();
    }
    return odds;
}

// ── analyze_loan — primary API entry point ────────────────────────────

LoanResponse StochasticSimulator::analyze_loan(
    size_t SSN,
    double amount,
    double max_interest_rate,
    Date start_date,
    Date end_date,
    int num_payments
) {
    const auto& paths = get_or_run_simulation(SSN, end_date);

    LoanResponse resp;

    if (paths.empty()) {
        resp.viable = false;
        return resp;
    }

    // Build milestone schedule
    double total_repayment = amount * (1.0 + max_interest_rate); // placeholder; overridden per rate below
    auto milestones = build_installment_schedule(amount, start_date, end_date, num_payments);

    // Recommended rate (best profit)
    resp.recommended_interest_rate = _best_rate_from_paths(paths, amount, milestones, start_date);

    // Minimum viable rate — sample 200 candidates from 0 to max
    std::vector<double> candidates;
    candidates.reserve(200);
    for (int i = 0; i <= 200; i++)
        candidates.push_back((max_interest_rate / 200.0) * i);
    resp.minimum_viable_rate = _min_rate_from_paths(paths, amount, candidates, milestones, start_date);

    // Statistics at recommended rate
    resp.statistics.repayment_probability =
        _odds_from_paths(paths, amount, resp.recommended_interest_rate, milestones, start_date) / 100.0;
    resp.statistics.estimated_profit =
        _profit_from_paths(paths, amount, resp.recommended_interest_rate, milestones, start_date);

    resp.viable = resp.statistics.estimated_profit > 0.0;

    // Milestone pass rates
    auto schedule_odds = _schedule_odds_from_paths(paths, milestones, start_date);
    for (int i = 0; i < (int)milestones.size(); i++) {
        resp.milestones.push_back({
            .date           = date_to_string(milestones[i].first),
            .payment_amount = milestones[i].second,
            .pass_rate      = schedule_odds[i] / 100.0
        });
    }

    // Interest rate sweep — 10 points from 0 to max_interest_rate
    int sweep_steps = 10;
    for (int s = 0; s <= sweep_steps; s++) {
        double rate = (max_interest_rate / sweep_steps) * s;
        auto sweep_milestones = build_installment_schedule(amount * (1.0 + rate), start_date, end_date, num_payments);
        double prob   = _odds_from_paths(paths, amount, rate, sweep_milestones, start_date) / 100.0;
        double profit = _profit_from_paths(paths, amount, rate, sweep_milestones, start_date);
        resp.interest_rate_sweep.push_back({rate, prob, profit});
    }

    // Sample 200 paths for animation, convert to cumulative balance
    auto cumulative = to_cumulative(paths);
    int sample_size = std::min(200, (int)cumulative.size());
    int step        = (int)cumulative.size() / sample_size;
    resp.paths.reserve(sample_size);
    for (int i = 0; i < sample_size; i++)
        resp.paths.push_back(cumulative[i * step]);

    return resp;
}

// ── stochastic_analysis_properties ───────────────────────────────────

std::vector<std::pair<double, double>> StochasticSimulator::stochastic_analysis_properties(size_t SSN, Date pay_date) {
    size_t job = 1, location = 1;
    try { job = DataBaseManager::get_current_job(SSN); location = DataBaseManager::get_current_location(SSN); } catch (...) {}
    return stochastic_analysis_properties(SSN, job, location, pay_date);
}

std::vector<std::pair<double, double>> StochasticSimulator::stochastic_analysis_properties(size_t SSN, size_t ocupation, size_t location, Date pay_date) {
    std::vector<std::vector<double>> simulation = stochastic_analysis(SSN, ocupation, location, pay_date);
    if (simulation.empty()) return {};

    auto size   = simulation.size();
    auto length = simulation[0].size();
    auto result = std::vector<std::pair<double, double>>(size);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)size; i++) {
        double avg = 0.0;
        for (int j = 0; j < (int)length; j++) avg += simulation[i][j];
        result[i].first = avg / length;
    }

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)size; i++) {
        double variance = 0.0;
        for (int j = 0; j < (int)length; j++) {
            double diff = simulation[i][j] - result[i].first;
            variance += diff * diff;
        }
        result[i].second = std::sqrt(variance / length);
    }
    return result;
}

// ── evaluate_milestones ───────────────────────────────────────────────

std::vector<bool> StochasticSimulator::evaluate_milestones(
    const std::vector<std::vector<double>>& paths,
    const std::vector<std::pair<_date, double>>& milestones,
    _date start_date,
    double interest_rate
) {
    std::vector<bool> results(paths.size(), false);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        double balance       = 0.0;
        double rollover      = 0.0;
        size_t milestone_idx = 0;

        for (size_t day = 0; day < paths[i].size(); day++) {
            balance += paths[i][day];
            if (milestone_idx < milestones.size()) {
                int milestone_day = milestones[milestone_idx].first - start_date;
                if ((int)day == milestone_day) {
                    double required = milestones[milestone_idx].second + rollover;
                    milestone_idx++;
                    if (balance >= required) { balance -= required; rollover = 0.0; }
                    else if (balance > 0.0)  { rollover = (required - balance) * (1.0 + interest_rate); balance = 0.0; }
                    else                     { rollover = required * (1.0 + interest_rate); }
                }
            }
        }
        results[i] = (milestone_idx == milestones.size() && rollover == 0.0);
    }
    return results;
}

// ── build_installment_schedule ────────────────────────────────────────

std::vector<std::pair<_date, double>> StochasticSimulator::build_installment_schedule(
    double total_repayment, _date start_date, _date end_date, int num_payments
) {
    std::vector<std::pair<_date, double>> milestones;
    double payment_amount = total_repayment / num_payments;
    int total_days        = end_date - start_date;
    int interval          = total_days / num_payments;

    for (int p = 0; p < num_payments; p++) {
        _date payment_date = start_date + _date{interval * (p + 1), 0, 0};
        milestones.push_back({payment_date, payment_amount});
    }
    milestones.back().first = end_date;
    return milestones;
}

// ── Public single-payment overloads (now use cache) ───────────────────

double StochasticSimulator::get_loan_payment_odds_percent(size_t SSN, double amount, double interest_rate, Date date) {
    const auto& paths = get_or_run_simulation(SSN, date);
    if (paths.empty()) return 0.0;
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);
    Date start{local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start, date, 1);
    return _odds_from_paths(paths, amount, interest_rate, milestones, start);
}

double StochasticSimulator::get_estimated_loan_profit(size_t SSN, double amount, double interest_rate, Date date) {
    const auto& paths = get_or_run_simulation(SSN, date);
    if (paths.empty()) return 0.0;
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);
    Date start{local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start, date, 1);
    return _profit_from_paths(paths, amount, interest_rate, milestones, start);
}

double StochasticSimulator::get_best_interest_for_profit(size_t SSN, double amount, Date date) {
    const auto& paths = get_or_run_simulation(SSN, date);
    if (paths.empty()) return 0.0;
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);
    Date start{local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    auto milestones = build_installment_schedule(amount, start, date, 1);
    return _best_rate_from_paths(paths, amount, milestones, start);
}

double StochasticSimulator::get_minimum_interest_for_profit(size_t SSN, double amount, std::vector<double> interest_rates, Date date) {
    const auto& paths = get_or_run_simulation(SSN, date);
    if (paths.empty()) return -1.0;
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);
    Date start{local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    auto milestones = build_installment_schedule(amount, start, date, 1);
    return _min_rate_from_paths(paths, amount, interest_rates, milestones, start);
}

// ── Milestone overloads ───────────────────────────────────────────────

double StochasticSimulator::get_loan_payment_odds_percent(size_t SSN, double amount, double interest_rate, const std::vector<std::pair<Date, double>>& milestones, Date start_date) {
    const auto& paths = get_or_run_simulation(SSN, milestones.back().first);
    if (paths.empty()) return 0.0;
    return _odds_from_paths(paths, amount, interest_rate, milestones, start_date);
}

double StochasticSimulator::get_estimated_loan_profit(size_t SSN, double amount, double interest_rate, const std::vector<std::pair<Date, double>>& milestones, Date start_date) {
    const auto& paths = get_or_run_simulation(SSN, milestones.back().first);
    if (paths.empty()) return 0.0;
    return _profit_from_paths(paths, amount, interest_rate, milestones, start_date);
}

double StochasticSimulator::get_best_interest_for_profit(size_t SSN, double amount, const std::vector<std::pair<Date, double>>& milestones, Date start_date) {
    const auto& paths = get_or_run_simulation(SSN, milestones.back().first);
    if (paths.empty()) return 0.0;
    return _best_rate_from_paths(paths, amount, milestones, start_date);
}

double StochasticSimulator::get_minimum_interest_for_profit(size_t SSN, double amount, std::vector<double> interest_rates, const std::vector<std::pair<Date, double>>& milestones, Date start_date) {
    const auto& paths = get_or_run_simulation(SSN, milestones.back().first);
    if (paths.empty()) return -1.0;
    return _min_rate_from_paths(paths, amount, interest_rates, milestones, start_date);
}

// ── Convenience schedule overloads ────────────────────────────────────

double StochasticSimulator::get_loan_payment_odds_percent(size_t SSN, double amount, double interest_rate, Date start_date, Date end_date, int num_payments) {
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start_date, end_date, num_payments);
    return get_loan_payment_odds_percent(SSN, amount, interest_rate, milestones, start_date);
}

double StochasticSimulator::get_estimated_loan_profit(size_t SSN, double amount, double interest_rate, Date start_date, Date end_date, int num_payments) {
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start_date, end_date, num_payments);
    return get_estimated_loan_profit(SSN, amount, interest_rate, milestones, start_date);
}

double StochasticSimulator::get_best_interest_for_profit(size_t SSN, double amount, Date start_date, Date end_date, int num_payments) {
    auto milestones = build_installment_schedule(amount, start_date, end_date, num_payments);
    return get_best_interest_for_profit(SSN, amount, milestones, start_date);
}

double StochasticSimulator::get_minimum_interest_for_profit(size_t SSN, double amount, std::vector<double> interest_rates, Date start_date, Date end_date, int num_payments) {
    auto milestones = build_installment_schedule(amount, start_date, end_date, num_payments);
    return get_minimum_interest_for_profit(SSN, amount, interest_rates, milestones, start_date);
}

std::vector<double> StochasticSimulator::get_payment_schedule_odds(size_t SSN, double amount, double interest_rate, Date start_date, Date end_date, int num_payments) {
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start_date, end_date, num_payments);
    const auto& paths = get_or_run_simulation(SSN, end_date);
    if (paths.empty()) return {};
    return _schedule_odds_from_paths(paths, milestones, start_date);
}
