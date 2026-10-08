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
        {"Default", "B001", 4},
        {"Default", "B002", 4},
        {"Default", "B001", 1},
        {"Default", "J001", 6},
        {"Default", "R001", 20},
        {"Default", "R002", 25},
        {"Default", "R002", 15},
        {"Default", "R002", 5},
        {"Default", "E001", 5},
        {"Default", "A001", 3},
        {"Default", "T001", 1},
        {"Default", "X999", 1}
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

    // =========================================================
    // Q9 — Department budgets
    // =========================================================

    std::cout << "\n=== Q9 Department Budgets ===\n";

    Budget csBudget(Money::of(3000));

    csBudget.setQuota(
        ResourceCategory::Book,
        {5, Money::of(1500)});

    csBudget.setTitleLimit(
        ResourceCategory::Book,
        2);

    Budget physicsBudget(Money::of(2000));

    physicsBudget.setQuota(
        ResourceCategory::Book,
        {2, Money::of(500)});

    physicsBudget.setTitleLimit(
        ResourceCategory::Book,
        1);

    AcquisitionManager departmentAcq(
        catalog,
        budget,
        5,
        10);

    departmentAcq.addDepartment(
        "Computer Science",
        csBudget);

    departmentAcq.addDepartment(
        "Physics",
        physicsBudget);

    std::cout
        << "Computer Science budget: "
        << csBudget.total()
        << "\n";

    std::cout
        << "Physics budget: "
        << physicsBudget.total()
        << "\n";

    auto departmentResults =
        departmentAcq.processBatch({
            {"Computer Science", "B001", 2},
            {"Physics", "B001", 1},
            {"Physics", "B002", 1},
            {"Physics", "B003", 1}
        });

    std::cout << "Department purchase results:\n";

    for (const auto& result : departmentResults) {
        std::cout
            << "  "
            << result.department
            << " -> "
            << result.resourceId
            << " x"
            << result.quantity
            << ": "
            << (result.approved ? "APPROVED" : "REJECTED");

        if (!result.approved) {
            std::cout
                << " (" << result.reason << ")";
        }

        std::cout << "\n";
    }

    std::cout
        << "Computer Science spent: "
        << csBudget.spent()
        << "\n";

    std::cout
        << "Physics spent: "
        << physicsBudget.spent()
        << "\n";

    std::cout
        << "Computer Science Book titles used: "
        << csBudget.titlesUsed(ResourceCategory::Book)
        << " / 2\n";

    std::cout
        << "Physics Book titles used: "
        << physicsBudget.titlesUsed(ResourceCategory::Book)
        << " / 1\n";

    std::cout
        << "B001 holdings after Q9: "
        << catalog.holdings("B001")
        << " copies\n";

    std::string departmentReason;

    std::cout
        << "Can Physics buy another B001? "
        << (departmentAcq.canPurchase(
                "Physics",
                "B001",
                1,
                &departmentReason)
                ? "yes"
                : "no")
        << "\n";

    std::cout
        << "Unknown department check: "
        << (departmentAcq.canPurchase(
                "Mathematics",
                "B001",
                1,
                &departmentReason)
                ? "yes"
                : "no")
        << " ("
        << departmentReason
        << ")\n";

    std::string cancellationReason;

    try {
    PurchaseRecord departmentOrder =
        departmentAcq.purchase(
            "Computer Science",
            "B002",
            1);

    std::cout
        << "Computer Science order #"
        << departmentOrder.orderNo
        << " purchased B002.\n";

    departmentAcq.cancel(
        departmentOrder.orderNo);

    std::cout
        << "After cancelling that order, Computer Science spent: "
        << csBudget.spent()
        << "\n";
    } catch (const std::exception& ex) {
    cancellationReason = ex.what();

    std::cout
        << "Expected Computer Science rejection: "
        << cancellationReason
        << "\n";
}

    std::cout
    << "\nQ9 department-aware report:\n";

    departmentAcq.printReport(std::cout);

    // =========================================================
    // Q10 — Year-end budget rollover
    // =========================================================

    std::cout << "\n=== Q10 Year-end Budget Rollover ===\n";

    // Use a separate budget so the rollover demonstration
    // is independent of the earlier Q1-Q9 examples.
    Budget yearEndBudget(Money::of(10000));

    yearEndBudget.setQuota(
        ResourceCategory::Book,
        {20, Money::of(6000)});

    yearEndBudget.setTitleLimit(
        ResourceCategory::Book,
        3);

    // This year's spending: 6000.
    // Therefore unspent amount = 4000.
    yearEndBudget.commit(
        ResourceCategory::Book,
        6,
        Money::of(6000));

    yearEndBudget.commitTitle(
        ResourceCategory::Book,
        "B001");

    std::cout << "Current year total: "
            << yearEndBudget.total()
            << "\n";

    std::cout << "Current year spent: "
            << yearEndBudget.spent()
            << "\n";

    std::cout << "Current year unspent: "
            << yearEndBudget.remaining()
            << "\n";

    std::cout << "Current year Book titles used: "
            << yearEndBudget.titlesUsed(
                    ResourceCategory::Book)
            << " / "
            << *yearEndBudget.titleLimitFor(
                    ResourceCategory::Book)
            << "\n";

    // Carry forward 50% of the unspent amount.
    // 50% of 4000 = 2000.
    Budget nextYearBudget = yearEndBudget.rollover(50);

    std::cout << "\n50% rollover to next year:\n";

    std::cout << "Next year total: "
            << nextYearBudget.total()
            << "\n";

    std::cout << "Next year spent: "
            << nextYearBudget.spent()
            << "\n";

    std::cout << "Next year remaining: "
            << nextYearBudget.remaining()
            << "\n";

    std::cout << "Next year Book quota: "
            << nextYearBudget.quotaFor(
                    ResourceCategory::Book)->maxUnits
            << " units / "
            << nextYearBudget.quotaFor(
                    ResourceCategory::Book)->maxSpend
            << "\n";

    std::cout << "Next year Book title limit: "
            << *nextYearBudget.titleLimitFor(
                    ResourceCategory::Book)
            << "\n";

    std::cout << "Next year Book units used: "
            << nextYearBudget.usageFor(
                    ResourceCategory::Book).units
            << "\n";

    std::cout << "Next year Book titles used: "
            << nextYearBudget.titlesUsed(
                    ResourceCategory::Book)
            << "\n";

         // =========================================================
    // Q11 — All-or-nothing batch processing
    // =========================================================

    std::cout
        << "\n=== Q11 All-or-Nothing Batch ===\n";

    /*
     * Use a separate catalogue and budget so Q11 is completely
     * independent of the earlier Q1-Q10 demonstrations.
     */
    Catalog q11Catalog;

    q11Catalog.emplace<Book>(
        "Q11-B1",
        "First Batch Book",
        std::vector<std::string>{"Author One"},
        "ISBN-Q11-1",
        "Publisher",
        2026,
        Money::of(100));

    q11Catalog.emplace<Book>(
        "Q11-B2",
        "Second Batch Book",
        std::vector<std::string>{"Author Two"},
        "ISBN-Q11-2",
        "Publisher",
        2026,
        Money::of(200));

    q11Catalog.emplace<Book>(
        "Q11-B3",
        "Third Batch Book",
        std::vector<std::string>{"Author Three"},
        "ISBN-Q11-3",
        "Publisher",
        2026,
        Money::of(300));

    Budget q11Budget(Money::of(2000));

    q11Budget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1500)});

    q11Budget.setTitleLimit(
        ResourceCategory::Book,
        3);

    AcquisitionManager q11Acq(
        q11Catalog,
        q11Budget);

    // ---------------------------------------------------------
    // Successful all-or-nothing batch
    // ---------------------------------------------------------

    std::cout
        << "\nSuccessful all-or-nothing batch:\n";

    auto q11Success =
        q11Acq.processBatch(
            {
                {"Default", "Q11-B1", 1},
                {"Default", "Q11-B2", 1}
            },
            true);

    for (const auto& result : q11Success) {
        std::cout
            << "  "
            << result.resourceId
            << " x"
            << result.quantity
            << ": "
            << (result.approved
                    ? "APPROVED"
                    : "REJECTED")
            << "\n";
    }

    std::cout
        << "Batch result count: "
        << q11Success.size()
        << "\n";

    std::cout
        << "Budget spent after successful batch: "
        << q11Budget.spent()
        << "\n";

    std::cout
        << "Book titles used: "
        << q11Budget.titlesUsed(
               ResourceCategory::Book)
        << " / 3\n";

    std::cout
        << "Q11-B1 holdings: "
        << q11Catalog.holdings("Q11-B1")
        << "\n";

    std::cout
        << "Q11-B2 holdings: "
        << q11Catalog.holdings("Q11-B2")
        << "\n";

    // ---------------------------------------------------------
    // Failed all-or-nothing batch
    // ---------------------------------------------------------

    std::cout
        << "\nFailed all-or-nothing batch:\n";

    const Money spentBefore =
        q11Budget.spent();

    const int b3Before =
        q11Catalog.holdings("Q11-B3");

    const int titlesBefore =
        q11Budget.titlesUsed(
            ResourceCategory::Book);

    const std::size_t historyBefore =
        q11Acq.history().size();

    auto q11Failure =
        q11Acq.processBatch(
            {
                {"Default", "Q11-B3", 1},
                {"Default", "Q11-UNKNOWN", 1}
            },
            true);

    std::cout
        << "Batch result count: "
        << q11Failure.size()
        << "\n";

    std::cout
        << "Q11-B3 holdings before failed batch: "
        << b3Before
        << "\n";

    std::cout
        << "Q11-B3 holdings after failed batch: "
        << q11Catalog.holdings("Q11-B3")
        << "\n";

    std::cout
        << "Budget spent before failed batch: "
        << spentBefore
        << "\n";

    std::cout
        << "Budget spent after failed batch: "
        << q11Budget.spent()
        << "\n";

    std::cout
        << "Book titles used before failed batch: "
        << titlesBefore
        << " / 3\n";

    std::cout
        << "Book titles used after failed batch: "
        << q11Budget.titlesUsed(
               ResourceCategory::Book)
        << " / 3\n";

    std::cout
        << "History size before failed batch: "
        << historyBefore
        << "\n";

    std::cout
        << "History size after failed batch: "
        << q11Acq.history().size()
        << "\n";

    std::cout
        << "All-or-nothing result: "
        << ((q11Failure.empty() &&
             q11Budget.spent() == spentBefore &&
             q11Catalog.holdings("Q11-B3") == b3Before &&
             q11Budget.titlesUsed(
                 ResourceCategory::Book) == titlesBefore &&
             q11Acq.history().size() == historyBefore)
                ? "NOTHING WAS BOUGHT"
                : "UNEXPECTED STATE")
        << "\n";


        // =========================================================
    // Q12 — Vendors
    // =========================================================

    std::cout
        << "\n=== Q12 Vendors ===\n";

    /*
     * Use a separate catalogue and budget so the vendor demonstration
     * is independent of the earlier Q1-Q11 examples.
     */
    Catalog q12Catalog;

    q12Catalog.emplace<Book>(
        "Q12-B1",
        "Vendor Demo Book",
        std::vector<std::string>{"Demo Author"},
        "ISBN-Q12-1",
        "Demo Publisher",
        2026,
        Money::of(500));

    q12Catalog.emplace<Book>(
        "Q12-B2",
        "Second Vendor Demo Book",
        std::vector<std::string>{"Second Author"},
        "ISBN-Q12-2",
        "Demo Publisher",
        2026,
        Money::of(800));

    Budget q12Budget(Money::of(5000));

    q12Budget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(4000)});

    q12Budget.setTitleLimit(
        ResourceCategory::Book,
        2);

    AcquisitionManager q12Acq(
        q12Catalog,
        q12Budget);

    // ---------------------------------------------------------
    // Register several vendors for the same title.
    // ---------------------------------------------------------

    q12Acq.addVendor(
        "Q12-B1",
        "Vendor A",
        Money::of(500));

    q12Acq.addVendor(
        "Q12-B1",
        "Vendor B",
        Money::of(420));

    q12Acq.addVendor(
        "Q12-B1",
        "Vendor C",
        Money::of(470));

    std::string q12Cheapest =
        q12Acq.cheapestVendor("Q12-B1");

    std::cout
        << "Q12-B1 cheapest vendor: "
        << q12Cheapest
        << "\n";

    std::cout
        << "Q12-B1 quote for 2 copies: "
        << q12Acq.quote("Q12-B1", 2)
        << "\n";

    // ---------------------------------------------------------
    // Purchase must automatically select Vendor B.
    // ---------------------------------------------------------

    PurchaseRecord q12First =
        q12Acq.purchase("Q12-B1", 2);

    std::cout
        << "Purchase Q12-B1 x2: "
        << (q12First.approved ? "APPROVED" : "REJECTED")
        << "\n";

    std::cout
        << "Vendor used: "
        << q12First.vendor
        << "\n";

    std::cout
        << "Pre-tax cost: "
        << q12First.preTaxCost
        << "\n";

    // ---------------------------------------------------------
    // Add an even cheaper vendor.
    // Future purchases must use it.
    // ---------------------------------------------------------

    q12Acq.addVendor(
        "Q12-B1",
        "Vendor D",
        Money::of(390));

    q12Cheapest =
        q12Acq.cheapestVendor("Q12-B1");

    std::cout
        << "\nAfter adding a cheaper vendor:\n";

    std::cout
        << "Q12-B1 cheapest vendor: "
        << q12Cheapest
        << "\n";

    PurchaseRecord q12Second =
        q12Acq.purchase("Q12-B1", 1);

    std::cout
        << "Purchase Q12-B1 x1: "
        << (q12Second.approved ? "APPROVED" : "REJECTED")
        << "\n";

    std::cout
        << "Vendor used: "
        << q12Second.vendor
        << "\n";

    // ---------------------------------------------------------
    // Different titles can have different cheapest vendors.
    // ---------------------------------------------------------

    q12Acq.addVendor(
        "Q12-B2",
        "Book Supplier",
        Money::of(800));

    q12Acq.addVendor(
        "Q12-B2",
        "Academic Supplier",
        Money::of(750));

    q12Cheapest =
        q12Acq.cheapestVendor("Q12-B2");

    std::cout
        << "\nQ12-B2 cheapest vendor: "
        << q12Cheapest
        << "\n";

    // ---------------------------------------------------------
    // Batch purchases also use the cheapest registered vendor.
    // ---------------------------------------------------------

    auto q12Batch =
        q12Acq.processBatch({
            {"Default", "Q12-B1", 1},
            {"Default", "Q12-B2", 1}
        });

    std::cout
        << "\nQ12 batch results:\n";

    for (const auto& result : q12Batch) {
        std::cout
            << "  "
            << result.resourceId
            << " x"
            << result.quantity
            << ": "
            << (result.approved
                    ? "APPROVED"
                    : "REJECTED");

        if (result.approved) {
            std::cout
                << ", vendor = "
                << result.vendor
                << ", cost = "
                << result.cost;
        } else {
            std::cout
                << " ("
                << result.reason
                << ")";
        }

        std::cout << "\n";
    }

    // ---------------------------------------------------------
    // Vendor information is preserved in order history.
    // ---------------------------------------------------------

    std::cout
        << "\nQ12 order history:\n";

    q12Acq.printReport(std::cout);

    std::cout
        << "\nQ12 final holdings:\n";

    std::cout
        << "  Q12-B1: "
        << q12Catalog.holdings("Q12-B1")
        << " copies\n";

    std::cout
        << "  Q12-B2: "
        << q12Catalog.holdings("Q12-B2")
        << " copies\n";

        // =========================================================
    // Q13 — Catalog Searches
    // =========================================================

    std::cout << "\n=== Q13 Catalog Searches ===\n";

    // ---------------------------------------------------------
    // Search by author
    // ---------------------------------------------------------

    std::cout << "\nSearch by author: Robert C. Martin\n";

    auto authorResults =
        catalog.searchAuthor("Robert C. Martin");

    for (const Resource* r : authorResults) {
        std::cout << "  " << r->id()
                  << " - " << r->title()
                  << "\n";
    }

    // ---------------------------------------------------------
    // Search by ISBN / ISSN
    // ---------------------------------------------------------

    std::cout << "\nSearch by ISBN: 978-0132350884\n";

    auto isbnResults =
        catalog.searchIsbnIssn("978-0132350884");

    for (const Resource* r : isbnResults) {
        std::cout << "  " << r->id()
                  << " - " << r->title()
                  << "\n";
    }

    std::cout << "\nSearch by ISSN: 0360-0300\n";

    auto issnResults =
        catalog.searchIsbnIssn("0360-0300");

    for (const Resource* r : issnResults) {
        std::cout << "  " << r->id()
                  << " - " << r->title()
                  << "\n";
    }

    // ---------------------------------------------------------
    // Search by publication-year range
    // ---------------------------------------------------------

    std::cout << "\nSearch publication years: 2013 to 2020\n";

    auto yearResults =
        catalog.searchYearRange(2013, 2020);

    for (const Resource* r : yearResults) {
        std::cout << "  " << r->id()
                  << " - " << r->title()
                  << " (" << r->year() << ")\n";
    }    
    return 0;
}