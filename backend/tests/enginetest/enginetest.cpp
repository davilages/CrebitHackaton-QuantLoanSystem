#include <iostream>
#include <cassert>
#include <cmath>
#include <iomanip>
#include "../../stocastic_simulation/stocastic_simulation.hpp"

// ── ANSI colors ───────────────────────────────────────────────────────
#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"

// ── Test helpers ──────────────────────────────────────────────────────
int passed = 0;
int failed = 0;

void pass(const std::string& name) {
    std::cout << GREEN << "[PASS] " << RESET << name << "\n";
    passed++;
}

void fail(const std::string& name, const std::string& reason) {
    std::cout << RED << "[FAIL] " << RESET << name << " — " << reason << "\n";
    failed++;
}

void section(const std::string& name) {
    std::cout << "\n" << YELLOW << "──── " << name << " ────" << RESET << "\n";
}

bool approx(double a, double b, double tol = 0.01) {
    return std::abs(a - b) < tol;
}

// ── Date sanity ───────────────────────────────────────────────────────
void test_date() {
    section("Date Arithmetic");

    // Forward and back
    Date today{8, 3, 2026};
    Date future = today + Date{30, 0, 0};
    int diff = future - today;
    if (diff == 30)
        pass("today + 30 days - today == 30");
    else
        fail("today + 30 days - today == 30", "got " + std::to_string(diff));

    // Negative delta (used in infer_start_date)
    Date past = today + Date{-30, 0, 0};
    int diff2 = today - past;
    if (diff2 == 30)
        pass("negative delta: today - (today - 30) == 30");
    else
        fail("negative delta: today - (today - 30) == 30", "got " + std::to_string(diff2));

    // Month boundary
    Date end_of_month{31, 3, 2026};
    Date next_day = end_of_month + Date{1, 0, 0};
    if (next_day.day == 1 && next_day.month == 4 && next_day.year == 2026)
        pass("month boundary: 31/03 + 1 == 01/04");
    else
        fail("month boundary", std::to_string(next_day.day) + "/" +
             std::to_string(next_day.month) + "/" + std::to_string(next_day.year));

    // Prefix increment
    Date d{28, 2, 2026};
    ++d;
    if (d.day == 1 && d.month == 3)
        pass("prefix ++: 28/02 -> 01/03");
    else
        fail("prefix ++: 28/02 -> 01/03", std::to_string(d.day) + "/" + std::to_string(d.month));
}

// ── Engine smoke tests ────────────────────────────────────────────────
void test_engine() {
    section("Engine — Stub predict_variation {0.0, 1.0}");

    StochasticSimulator sim;

    // Today + 90 days
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);
    Date today{local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    Date end_date = today + Date{90, 0, 0};

    size_t SSN = 99999999999ULL; // fake SSN — stub returns {0.0, 1.0} for all

    // ── odds in valid range ───────────────────────────────────────────
    double odds = sim.get_loan_payment_odds_percent(SSN, 1000.0, 0.10, end_date);
    std::cout << "  odds (single payment, 90d, 10%): " << std::fixed << std::setprecision(2) << odds << "%\n";
    if (odds >= 0.0 && odds <= 100.0)
        pass("odds in [0, 100]");
    else
        fail("odds in [0, 100]", std::to_string(odds));

    // ── pure random walk: odds should not be extreme ──────────────────
    if (odds > 5.0 && odds < 95.0)
        pass("pure random walk odds not degenerate");
    else
        fail("pure random walk odds not degenerate", std::to_string(odds) + "% — check simulation logic");

    // ── profit sign makes sense ───────────────────────────────────────
    double profit = sim.get_estimated_loan_profit(SSN, 1000.0, 0.10, end_date);
    std::cout << "  estimated profit (1000, 10%, 90d): " << std::fixed << std::setprecision(2) << profit << "\n";
    if (profit > -1000.0 && profit < 1000.0)
        pass("profit in reasonable range");
    else
        fail("profit in reasonable range", std::to_string(profit));

    // ── best rate is a valid rate ─────────────────────────────────────
    double best = sim.get_best_interest_for_profit(SSN, 1000.0, end_date);
    std::cout << "  best interest rate: " << std::fixed << std::setprecision(4) << best * 100.0 << "%\n";
    if (best >= 0.0 && best <= 1.0)
        pass("best rate in [0, 1]");
    else
        fail("best rate in [0, 1]", std::to_string(best));

    // ── minimum viable rate from a set ───────────────────────────────
    std::vector<double> rates = {0.05, 0.10, 0.15, 0.20, 0.25, 0.30};
    double min_rate = sim.get_minimum_interest_for_profit(SSN, 1000.0, rates, end_date);
    std::cout << "  minimum viable rate: ";
    if (min_rate < 0.0)
        std::cout << "none found\n";
    else
        std::cout << std::fixed << std::setprecision(4) << min_rate * 100.0 << "%\n";
    if (min_rate == -1.0 || (min_rate >= 0.0 && min_rate <= 1.0))
        pass("minimum viable rate valid or -1");
    else
        fail("minimum viable rate valid or -1", std::to_string(min_rate));

    // ── installment loan ──────────────────────────────────────────────
    section("Engine — Installment Loan (3 payments)");

    double odds_inst = sim.get_loan_payment_odds_percent(SSN, 1000.0, 0.10, today, end_date, 3);
    std::cout << "  installment odds (3 payments, 90d, 10%): " << std::fixed << std::setprecision(2) << odds_inst << "%\n";
    if (odds_inst >= 0.0 && odds_inst <= 100.0)
        pass("installment odds in [0, 100]");
    else
        fail("installment odds in [0, 100]", std::to_string(odds_inst));

    // Installment should be harder to pass than single payment
    // because there are intermediate checkpoints
    std::cout << "  single vs installment: " << odds << "% vs " << odds_inst << "%\n";
    if (odds_inst <= odds + 5.0) // small tolerance for randomness
        pass("installment odds <= single payment odds");
    else
        fail("installment odds <= single payment odds",
             "installment=" + std::to_string(odds_inst) + " single=" + std::to_string(odds));

    // ── edge: pay date in the past ────────────────────────────────────
    section("Engine — Edge Cases");

    Date past_date = today + Date{-10, 0, 0};
    double odds_past = sim.get_loan_payment_odds_percent(SSN, 1000.0, 0.10, past_date);
    std::cout << "  odds for past date: " << odds_past << "%\n";
    if (odds_past == 0.0)
        pass("past date returns 0%");
    else
        fail("past date returns 0%", std::to_string(odds_past));

    // ── edge: zero amount ─────────────────────────────────────────────
    double odds_zero = sim.get_loan_payment_odds_percent(SSN, 0.0, 0.10, end_date);
    std::cout << "  odds for zero amount: " << odds_zero << "%\n";
    if (odds_zero >= 0.0 && odds_zero <= 100.0)
        pass("zero amount does not crash");
    else
        fail("zero amount does not crash", std::to_string(odds_zero));

    // ── edge: very short loan (1 day) ─────────────────────────────────
    Date tomorrow = today + Date{1, 0, 0};
    double odds_short = sim.get_loan_payment_odds_percent(SSN, 1000.0, 0.10, tomorrow);
    std::cout << "  odds for 1 day loan: " << odds_short << "%\n";
    if (odds_short >= 0.0 && odds_short <= 100.0)
        pass("1 day loan does not crash");
    else
        fail("1 day loan does not crash", std::to_string(odds_short));
}

// ── Full output summary (mimics what frontend will receive) ───────────
void demo_output() {
    section("Demo Output — What Frontend Will See");

    StochasticSimulator sim;

    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);
    Date today{local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    Date end_date = today + Date{90, 0, 0};

    size_t SSN    = 99999999999ULL;
    double amount = 5000.0;

    std::vector<double> rate_options = {0.05, 0.08, 0.10, 0.12, 0.15, 0.18, 0.20, 0.25, 0.30};

    std::cout << "\n  Loan amount:  R$ " << amount << "\n";
    std::cout << "  Term:         90 days\n";
    std::cout << "  Payments:     3 installments\n\n";

    std::cout << "  Rate options:\n";
    std::cout << "  " << std::left
              << std::setw(10) << "Rate"
              << std::setw(12) << "Odds"
              << std::setw(16) << "Est. Profit"
              << "\n";
    std::cout << "  " << std::string(36, '-') << "\n";

    for (double rate : rate_options) {
        double odds   = sim.get_loan_payment_odds_percent(SSN, amount, rate, today, end_date, 3);
        double profit = sim.get_estimated_loan_profit(SSN, amount, rate, today, end_date, 3);
        std::cout << "  "
                  << std::setw(10) << (std::to_string((int)(rate * 100)) + "%")
                  << std::setw(12) << (std::to_string((int)odds) + "%")
                  << std::setw(16) << ("R$ " + std::to_string((int)profit))
                  << "\n";
    }

    double best = sim.get_best_interest_for_profit(SSN, amount, today, end_date, 3);
    double min  = sim.get_minimum_interest_for_profit(SSN, amount, rate_options, today, end_date, 3);

    std::cout << "\n  Best rate for maximum profit: " << (int)(best * 100) << "%\n";
    if (min >= 0.0)
        std::cout << "  Minimum viable rate:          " << (int)(min * 100) << "%\n";
    else
        std::cout << "  Minimum viable rate:          none found in provided set\n";
}

// ── main ──────────────────────────────────────────────────────────────
int main() {
    std::cout << "\n=== StochasticSimulator Test Suite ===\n";

    test_date();
    test_engine();
    demo_output();

    std::cout << "\n=== Results: "
              << GREEN << passed << " passed" << RESET << ", "
              << (failed > 0 ? RED : "")
              << failed << " failed"
              << RESET << " ===\n\n";

    return failed > 0 ? 1 : 0;
}