// Demo: builds a small catalog, sets a budget with per-category quotas,
// title limits and runs a batch of purchase requests through the
// acquisition manager.

#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    Catalog catalog;

    catalog.emplace<Book>(
        "B001",
        "Clean Code",
        std::vector<std::string>{"Robert C. Martin"},
        "978-0132350884",
        "Prentice Hall",
        2008,
        Money::of(450));

    catalog.emplace<Book>(
        "B002",
        "The C++ Programming Language",
        std::vector<std::string>{"Bjarne Stroustrup"},
        "978-0321563842",
        "Addison-Wesley",
        2013,
        Money::of(1200),
        4,
        Binding::Hardcover);

    // Q7 demonstration resource: third Book title.
    catalog.emplace<Book>(
        "B003",
        "Effective Modern C++",
        std::vector<std::string>{"Scott Meyers"},
        "978-1491903995",
        "O'Reilly",
        2014,
        Money::of(100));

    catalog.emplace<Journal>(
        "J001",
        "ACM Computing Surveys",
        "0360-0300",
        4,
        "ACM",
        2026,
        Money::of(100),
        1);

    catalog.emplace<EBook>(
        "E001",
        "Modern C++",
        std::vector<std::string>{"Bjarne Stroustrup"},
        "978-0321563842",
        "Tech Press",
        2026,
        Money::of(300),
        "https://ebooks.example/modern-cpp",
        LicenseModel::AnnualSubscription,
        Money::of(500),
        FileFormat::EPUB,
        true);

    catalog.emplace<AudioBook>(
        "A001",
        "C++ Audio Guide",
        "Jane Smith",
        360,
        "Audio Press",
        2026,
        Money::of(200),
        "https://audio.example/cpp",
        LicenseModel::AnnualSubscription,
        Money::of(300));

    catalog.emplace<Thesis>(
        "T001",
        "Efficient Algorithms",
        "ABC University",
        "M.Tech",
        "Dr. Rao",
        2026);

    catalog.emplace<ElectronicResource>(
        "R001",
        "IEEE Xplore Digital Library",
        "IEEE",
        2026,
        Money::of(150),
        "https://ieeexplore.example",
        LicenseModel::AnnualSubscription,
        Money::of(2000));

    catalog.emplace<ElectronicResource>(
        "R002",
        "MATLAB Campus Licence",
        "MathWorks",
        2026,
        Money::of(400),
        "https://licensing.example/matlab",
        LicenseModel::Perpetual);

    std::cout << "=== Catalog ===\n";

    for (const Resource* r : catalog.all()) {
        std::cout << r->summary() << "\n";
    }

    std::cout << "\n=== Details of R001 ===\n"
              << catalog.get("R001");

    Budget budget(Money::of(25000));

    budget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(8000)});

    budget.setQuota(
        ResourceCategory::Journal,
        {6, Money::of(5000)});

    budget.setQuota(
        ResourceCategory::EBook,
        {5, Money::of(2500)});

    budget.setQuota(
        ResourceCategory::AudioBook,
        {5, Money::of(2500)});

    budget.setQuota(
        ResourceCategory::Thesis,
        {10, Money::of(1000)});

    budget.setQuota(
        ResourceCategory::ElectronicResource,
        {40, Money::of(13000)});

    // =========================================================
    // Q7 — Different-title limits
    // =========================================================

    // Only two different Book titles may be purchased.
    budget.setTitleLimit(
        ResourceCategory::Book,
        2);

    // One Journal title may be purchased.
    budget.setTitleLimit(
        ResourceCategory::Journal,
        1);

    // Two different ElectronicResource titles may be purchased.
    budget.setTitleLimit(
        ResourceCategory::ElectronicResource,
        2);

    // One title in each of these categories.
    budget.setTitleLimit(
        ResourceCategory::EBook,
        1);

    budget.setTitleLimit(
        ResourceCategory::AudioBook,
        1);

    budget.setTitleLimit(
        ResourceCategory::Thesis,
        1);

    // Q6: configurable tax rates.
    // Print resources: 5%
    // Electronic resources: 10%
    AcquisitionManager acq(
        catalog,
        budget,
        5,
        10);

    std::cout << "\n=== Q6 Tax Configuration ===\n";

    std::cout << "Print tax rate: "
              << acq.printTaxPercent()
              << "%\n";

    std::cout << "Electronic tax rate: "
              << acq.electronicTaxPercent()
              << "%\n";

    std::cout << "B001, 1 copy pre-tax: "
              << Money::of(450)
              << ", post-tax quote: "
              << acq.quote("B001", 1)
              << "\n";

    std::cout << "E001, 1 seat pre-tax: 800.00, "
              << "post-tax quote "
              << acq.quote("E001", 1)
              << "\n";

    std::cout << "\n=== Q7 Title Limits ===\n";

    std::cout << "Book title limit: "
              << *budget.titleLimitFor(
                     ResourceCategory::Book)
              << "\n";

    std::cout << "Journal title limit: "
              << *budget.titleLimitFor(
                     ResourceCategory::Journal)
              << "\n";

    std::cout << "Book titles currently used: "
              << budget.titlesUsed(
                     ResourceCategory::Book)
              << "\n";

    std::string titleReason;

    std::cout << "Can purchase B001? "
              << (acq.canPurchase(
                      "B001",
                      1,
                      &titleReason)
                      ? "yes"
                      : "no")
              << "\n";

    std::cout << "Can purchase B003 before any "
                 "Book purchase? "
              << (budget.checkTitle(
                      ResourceCategory::Book,
                      "B003")
                      .empty()
                      ? "yes"
                      : "no")
              << "\n";

    std::cout << "\n=== Quotes ===\n";

    std::cout << "5 copies of B002 = "
              << acq.quote("B002", 5)
              << "\n";

    std::cout << "2 copies of J001 for 1 year = "
              << acq.quote("J001", 2)
              << "\n";

    std::cout << "5 seats of E001 = "
              << acq.quote("E001", 5)
              << "  (EBook)\n";

    std::cout << "3 seats of A001 = "
              << acq.quote("A001", 3)
              << "  (AudioBook)\n";

    std::cout << "1 copy of T001 = "
              << acq.quote("T001", 1)
              << "  (Thesis, usually free)\n";

    std::cout << "20 seats of R001 = "
              << acq.quote("R001", 20)
              << "  (incl. platform fee)\n";

    std::cout
        << "Q5 bulk discount quotes "
           "(now including Q6 tax):\n";

    std::cout
        << "  10 copies of B002 "
           "(hardcover + bulk discount): "
        << acq.quote("B002", 10)
        << "\n";

    std::cout
        << "  60 seats of R001 "
           "(first 50 full price, remaining half price): "
        << acq.quote("R001", 60)
        << "\n";

    // =========================================================
    // Existing Q1-Q6 purchase demonstration
    // =========================================================

    acq.processBatch({
        {"B001", 4},
        {"B002", 4},
        {"B001", 1},
        {"J001", 6},
        {"R001", 20},
        {"R002", 25},
        {"R002", 15},
        {"R002", 5},
        {"E001", 5},
        {"A001", 3},
        {"T001", 1},
        {"X999", 1}
    });

    std::cout << "\n=== Acquisition report ===\n";

    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";

    budget.print(std::cout);

    std::cout << "\n=== Title Usage ===\n";

    std::cout << "Book titles used: "
              << budget.titlesUsed(
                     ResourceCategory::Book)
              << " / "
              << *budget.titleLimitFor(
                     ResourceCategory::Book)
              << "\n";

    std::cout << "Journal titles used: "
              << budget.titlesUsed(
                     ResourceCategory::Journal)
              << " / "
              << *budget.titleLimitFor(
                     ResourceCategory::Journal)
              << "\n";

    std::cout << "ElectronicResource titles used: "
              << budget.titlesUsed(
                     ResourceCategory::ElectronicResource)
              << " / "
              << *budget.titleLimitFor(
                     ResourceCategory::ElectronicResource)
              << "\n";

    std::cout << "\n=== Holdings ===\n";

    for (const Resource* r : catalog.all()) {
        std::cout
            << "  "
            << r->id()
            << ": "
            << catalog.holdings(r->id())
            << (r->isDigital() ? " seats" : " copies")
            << "\n";
    }

    // =========================================================
    // Q7 — Buying an existing title again
    // =========================================================

    std::cout
        << "\n=== Q7 Buying an existing Book title again ===\n";

    Budget repeatTitleBudget(Money::of(1000));

    repeatTitleBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000)});

    repeatTitleBudget.setTitleLimit(
        ResourceCategory::Book,
        2);

    AcquisitionManager repeatTitleAcq(
        catalog,
        repeatTitleBudget,
        5,
        10);

    repeatTitleAcq.purchase("B001", 1);

    std::cout
        << "After first B001 purchase: "
        << repeatTitleBudget.titlesUsed(
               ResourceCategory::Book)
        << " / 2 titles used\n";

    try {
        repeatTitleAcq.purchase("B001", 1);

        std::cout
            << "B001 approved again: existing title "
               "does not consume another title slot.\n";

    } catch (const std::exception& e) {

        std::cout
            << "Unexpected rejection: "
            << e.what()
            << "\n";
    }

    std::cout
        << "After second B001 purchase: "
        << repeatTitleBudget.titlesUsed(
               ResourceCategory::Book)
        << " / 2 titles used\n";

    // =========================================================
    // Q8 — Cancellation
    // =========================================================

    std::cout << "\n=== Q8 Cancellation ===\n";

    // Use a fresh budget so the cancellation effects are easy to see.
    Budget cancelBudget(Money::of(5000));

    cancelBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(5000)});

    cancelBudget.setTitleLimit(
        ResourceCategory::Book,
        2);

    AcquisitionManager cancelAcq(
        catalog,
        cancelBudget,
        10,
        0);

    PurchaseRecord q8First =
        cancelAcq.purchase("B001", 2);

    PurchaseRecord q8Second =
        cancelAcq.purchase("B001", 1);

    PurchaseRecord q8Other =
        cancelAcq.purchase("B002", 1);

    std::cout << "Active B002 order: #"
              << q8Other.orderNo << "\n";

    std::cout << "Before cancellation:\n";

    std::cout << "  Budget spent: "
              << cancelBudget.spent()
              << "\n";

    std::cout << "  Book units used: "
              << cancelBudget.usageFor(
                     ResourceCategory::Book).units
              << "\n";

    std::cout << "  B001 holdings: "
              << catalog.holdings("B001")
              << " copies\n";

    std::cout << "  Book titles used: "
              << cancelBudget.titlesUsed(
                     ResourceCategory::Book)
              << " / 2\n";

    cancelAcq.cancel(q8First.orderNo);

    std::cout << "\nAfter cancelling order #"
              << q8First.orderNo
              << ":\n";

    std::cout << "  Budget spent: "
              << cancelBudget.spent()
              << "\n";

    std::cout << "  Book units used: "
              << cancelBudget.usageFor(
                     ResourceCategory::Book).units
              << "\n";

    std::cout << "  B001 holdings: "
              << catalog.holdings("B001")
              << " copies\n";

    std::cout << "  Book titles used: "
              << cancelBudget.titlesUsed(
                     ResourceCategory::Book)
              << " / 2\n";

    std::cout << "\nCancelling the second B001 order...\n";

    cancelAcq.cancel(q8Second.orderNo);

    std::cout << "After cancelling the final active B001 order:\n";

    std::cout << "  Budget spent: "
              << cancelBudget.spent()
              << "\n";

    std::cout << "  B001 holdings: "
              << catalog.holdings("B001")
              << " copies\n";

    std::cout << "  Book titles used: "
              << cancelBudget.titlesUsed(
                     ResourceCategory::Book)
              << " / 2\n";

    // The B001 title slot is now free, so B003 can be purchased.
    cancelAcq.purchase("B003", 1);

    std::cout
        << "B003 approved after B001 title slot was released.\n";

    std::cout << "  Book titles used: "
              << cancelBudget.titlesUsed(
                     ResourceCategory::Book)
              << " / 2\n";

    std::cout << "\nQ8 cancellation history:\n";

    cancelAcq.printReport(std::cout);

    std::cout << "\nQ8 final holdings:\n";

    std::cout << "  B001: "
              << catalog.holdings("B001")
              << " copies\n";

    std::cout << "  B002: "
              << catalog.holdings("B002")
              << " copies\n";

    std::cout << "  B003: "
              << catalog.holdings("B003")
              << " copies\n";

    return 0;
}