#include "stocastic_simulation.hpp"
#include "./database_management/database_manager.hpp"
#include "./datatypes.hpp"
#include <immintrin.h>

#include <iostream>
#include <random>
#include <chrono>

#include <Eigen/Dense> // Matirx Operations
#include <omp.h> // Concurrent / Parallel programming

std::pair<double, double> StochasticSimulator::predict_variation(MC& m) {
    if (records.find(m) != records.end())
        return records[m];
    return {0.0, 1.0};
}

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, Date pay_date) {
    auto job = DataBaseManager::get_current_job(SSN);
    auto location = DataBaseManager::get_current_location(SSN);

    return stochastic_analysis(SSN, job, location, max_date)  ;
}

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, size_t ocupation, size_t location, Date pay_date) {

    // ── Step 0.1: date setup ──────────────────────────────────────────
    auto now      = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);

    _date curr_date{local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    int m = pay_date - curr_date;

    if (m <= 0) return {};

    // ── Step 0.2: preallocate ─────────────────────────────────────────
    std::vector<std::pair<double, double>> records(m);

    auto return_value = std::vector<std::vector<double>>(
        N_SIMULATIONS, std::vector<double>(m, 0.0)
    );

    // ── Step 0.3: payday lookup ───────────────────────────────────────
    auto pay_days = DataBaseManager::get_pay_days({1, SSN, location, ocupation});

    // ── Step 1: prefetch all (mean, SD) into records array ───────────
    // Each index written exactly once, no two threads share an index
    // OpenMP barrier after this loop guarantees Step 2 sees all writes
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < m; i++) {
        _date target = curr_date + _date{i, 0, 0};

        auto data = MC{
            .day            = target.day,
            .month          = target.month,
            .year           = target.year,
            .day_of_week    = day_of_week(target),
            .location_id    = location,
            .profession_id  = ocupation
        };

        records[i] = predict_variation(data);
    }

    // ── Step 2: simulate N_SIMULATIONS paths ─────────────────────────
    // Parallel over paths, sequential within each path
    // records is read-only here — no synchronization needed
    #pragma omp parallel
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count()
                        ^ (std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::default_random_engine generator(seed);
        std::normal_distribution<double> normal(0.0, 1.0);

        #pragma omp for schedule(static)
        for (int i = 0; i < (int)N_SIMULATIONS; i++) {
            _date date = curr_date;

            for (int j = 0; j < m; j++) {
                auto [mean, sd] = records[j];

                double payday_injection = 0.0;
                auto it = pay_days.find(date);
                if (it != pay_days.end()) {
                    payday_injection = it->second;
                }

                return_value[i][j] = normal(generator) * sd + mean + payday_injection;

                ++date;
            }
        }
    }

    return return_value;
}

std::vector<std::pair<double, double>> StochasticSimulator::stochastic_analysis_properties(size_t SSN, Date pay_date) {
    auto job      = DataBaseManager::get_current_job(SSN);
    auto location = DataBaseManager::get_current_location(SSN);
    return stochastic_analysis_properties(SSN, job, location, pay_date);
}

std::vector<std::pair<double, double>> StochasticSimulator::stochastic_analysis_properties(size_t SSN, size_t ocupation, size_t location, Date pay_date) {
    std::vector<std::vector<double>> simulation = stochastic_analysis(SSN, ocupation, location, pay_date);

    if (simulation.empty()) return {};

    auto size   = simulation.size();
    auto length = simulation[0].size();
    auto result = std::vector<std::pair<double, double>>(size);

    // ── Pass 1: compute mean per path ─────────────────────────────────
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)size; i++) {
        double avg = 0.0;
        for (int j = 0; j < (int)length; j++) {
            avg += simulation[i][j];
        }
        result[i].first = avg / length;
    }

    // ── Pass 2: compute SD per path ───────────────────────────────────
    // Bug fixed: original was missing the squared term
    // SD = sqrt( sum((x - mean)^2) / length )
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

                    if (balance >= required) {
                        balance  -= required;
                        rollover  = 0.0;
                    } else if (balance > 0.0) {
                        rollover  = (required - balance) * (1.0 + interest_rate);
                        balance   = 0.0;
                    } else {
                        rollover  = required * (1.0 + interest_rate);
                    }
                }
            }
        }

        // Path succeeds if all milestones were reached and rollover is cleared
        results[i] = (milestone_idx == milestones.size() && rollover == 0.0);
    }

    return results;
}

std::vector<std::pair<_date, double>> StochasticSimulator::build_installment_schedule(
    double total_repayment,
    _date start_date,
    _date end_date,
    int num_payments
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

// ── shared helpers ────────────────────────────────────────────────────

static double compute_odds(const std::vector<bool>& results) {
    int successes = 0;
    #pragma omp parallel for reduction(+:successes)
    for (int i = 0; i < (int)results.size(); i++) {
        if (results[i]) successes++;
    }
    return (100.0 * successes) / (double)results.size();
}

static double compute_profit(
    const std::vector<bool>& results,
    double amount,
    double interest_rate
) {
    int successes = 0;
    #pragma omp parallel for reduction(+:successes)
    for (int i = 0; i < (int)results.size(); i++) {
        if (results[i]) successes++;
    }
    int failures = (int)results.size() - successes;
    return (successes * amount * interest_rate) + (failures * (-amount));
}

static _date infer_start_date(const std::vector<std::vector<double>>& paths, _date end_date) {
    return end_date + _date{-(int)paths[0].size(), 0, 0};
}

// ── single payment ────────────────────────────────────────────────────

double StochasticSimulator::get_loan_payment_odds_percent(
    size_t SSN, double amount, double interest_rate, _date date
) {
    auto paths      = stochastic_analysis(SSN, date);
    _date start     = infer_start_date(paths, date);
    std::vector<std::pair<_date, double>> milestones = {{date, amount * (1.0 + interest_rate)}};
    auto results    = evaluate_milestones(paths, milestones, start, interest_rate);
    return compute_odds(results);
}

double StochasticSimulator::get_estimated_loan_profit(
    size_t SSN, double amount, double interest_rate, _date date
) {
    auto paths      = stochastic_analysis(SSN, date);
    _date start     = infer_start_date(paths, date);
    std::vector<std::pair<_date, double>> milestones = {{date, amount * (1.0 + interest_rate)}};
    auto results    = evaluate_milestones(paths, milestones, start, interest_rate);
    return compute_profit(results, amount, interest_rate);
}

double StochasticSimulator::get_best_interest_for_profit(
    size_t SSN, double amount, _date date
) {
    auto paths      = stochastic_analysis(SSN, date);
    _date start     = infer_start_date(paths, date);

    double best_rate   = 0.0;
    double best_profit = std::numeric_limits<double>::lowest();

    for (double rate = 0.0; rate <= 1.0; rate += 0.005) {
        std::vector<std::pair<_date, double>> milestones = {{date, amount * (1.0 + rate)}};
        auto results = evaluate_milestones(paths, milestones, start, rate);
        double profit = compute_profit(results, amount, rate);
        if (profit > best_profit) {
            best_profit = profit;
            best_rate   = rate;
        }
    }

    return best_rate;
}

double StochasticSimulator::get_minimum_interest_for_profit(
    size_t SSN, double amount, std::vector<double> interest_rates, _date date
) {
    auto paths  = stochastic_analysis(SSN, date);
    _date start = infer_start_date(paths, date);

    for (double rate : interest_rates) {
        std::vector<std::pair<_date, double>> milestones = {{date, amount * (1.0 + rate)}};
        auto results = evaluate_milestones(paths, milestones, start, rate);
        if (compute_profit(results, amount, rate) > 0.0) return rate;
    }

    return -1.0;
}

// ── explicit milestones ───────────────────────────────────────────────

double StochasticSimulator::get_loan_payment_odds_percent(
    size_t SSN, double amount, double interest_rate,
    const std::vector<std::pair<_date, double>>& milestones, _date start_date
) {
    auto paths   = stochastic_analysis(SSN, milestones.back().first);
    auto results = evaluate_milestones(paths, milestones, start_date, interest_rate);
    return compute_odds(results);
}

double StochasticSimulator::get_estimated_loan_profit(
    size_t SSN, double amount, double interest_rate,
    const std::vector<std::pair<_date, double>>& milestones, _date start_date
) {
    auto paths   = stochastic_analysis(SSN, milestones.back().first);
    auto results = evaluate_milestones(paths, milestones, start_date, interest_rate);
    return compute_profit(results, amount, interest_rate);
}

double StochasticSimulator::get_best_interest_for_profit(
    size_t SSN, double amount,
    const std::vector<std::pair<_date, double>>& milestones, _date start_date
) {
    auto paths = stochastic_analysis(SSN, milestones.back().first);

    double best_rate   = 0.0;
    double best_profit = std::numeric_limits<double>::lowest();

    for (double rate = 0.0; rate <= 1.0; rate += 0.005) {
        auto scaled            = milestones;
        double per_payment     = (amount * (1.0 + rate)) / (double)scaled.size();
        for (auto& m : scaled) m.second = per_payment;

        auto results  = evaluate_milestones(paths, scaled, start_date, rate);
        double profit = compute_profit(results, amount, rate);
        if (profit > best_profit) {
            best_profit = profit;
            best_rate   = rate;
        }
    }

    return best_rate;
}

double StochasticSimulator::get_minimum_interest_for_profit(
    size_t SSN, double amount, std::vector<double> interest_rates,
    const std::vector<std::pair<_date, double>>& milestones, _date start_date
) {
    auto paths = stochastic_analysis(SSN, milestones.back().first);

    for (double rate : interest_rates) {
        auto scaled            = milestones;
        double per_payment     = (amount * (1.0 + rate)) / (double)scaled.size();
        for (auto& m : scaled) m.second = per_payment;

        auto results = evaluate_milestones(paths, scaled, start_date, rate);
        if (compute_profit(results, amount, rate) > 0.0) return rate;
    }

    return -1.0;
}

// ── convenience ───────────────────────────────────────────────────────

double StochasticSimulator::get_loan_payment_odds_percent(
    size_t SSN, double amount, double interest_rate,
    _date start_date, _date end_date, int num_payments
) {
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start_date, end_date, num_payments);
    return get_loan_payment_odds_percent(SSN, amount, interest_rate, milestones, start_date);
}

double StochasticSimulator::get_estimated_loan_profit(
    size_t SSN, double amount, double interest_rate,
    _date start_date, _date end_date, int num_payments
) {
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start_date, end_date, num_payments);
    return get_estimated_loan_profit(SSN, amount, interest_rate, milestones, start_date);
}

double StochasticSimulator::get_best_interest_for_profit(
    size_t SSN, double amount,
    _date start_date, _date end_date, int num_payments
) {
    auto milestones = build_installment_schedule(amount, start_date, end_date, num_payments);
    return get_best_interest_for_profit(SSN, amount, milestones, start_date);
}

double StochasticSimulator::get_minimum_interest_for_profit(
    size_t SSN, double amount, std::vector<double> interest_rates,
    _date start_date, _date end_date, int num_payments
) {
    auto milestones = build_installment_schedule(amount, start_date, end_date, num_payments);
    return get_minimum_interest_for_profit(SSN, amount, interest_rates, milestones, start_date);
}
