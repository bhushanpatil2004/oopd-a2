// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.

#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    Catalog catalog;

    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover);
    catalog.emplace<Journal>("J001","ACM Computing Surveys","0360-0300",4,"ACM",2026,Money::of(100),1);
    catalog.emplace<EBook>("E001","Modern C++",std::vector<std::string>{"Bjarne Stroustrup"},"978-0321563842",
                            "Tech Press",2026,Money::of(300),"https://ebooks.example/modern-cpp",LicenseModel::AnnualSubscription,
                            Money::of(500),FileFormat::EPUB, true);
    catalog.emplace<AudioBook>( "A001", "C++ Audio Guide", "Jane Smith", 360, "Audio Press", 2026, Money::of(200),
                                 "https://audio.example/cpp",LicenseModel::AnnualSubscription,Money::of(300));

    catalog.emplace<Thesis>("T001","Efficient Algorithms", "ABC University", "M.Tech", "Dr. Rao", 2026);
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    catalog.emplace<ElectronicResource>("R002", "MATLAB Campus Licence", "MathWorks", 2026,
                                        Money::of(400), "https://licensing.example/matlab",
                                        LicenseModel::Perpetual);

    std::cout << "=== Catalog ===\n";
    for (const Resource* r : catalog.all()) std::cout << r->summary() << "\n";

    std::cout << "\n=== Details of R001 ===\n" << catalog.get("R001");

    Budget budget(Money::of(25000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(8000)});
    budget.setQuota(ResourceCategory::Journal, {6, Money::of(5000)});
    budget.setQuota(ResourceCategory::EBook,{5, Money::of(2500)});
    budget.setQuota(ResourceCategory::AudioBook, {5, Money::of(2500)});
    budget.setQuota(ResourceCategory::Thesis, {10, Money::of(1000)});
    budget.setQuota(ResourceCategory::ElectronicResource, {40, Money::of(13000)});

    // Q6: configurable tax rates.
    // Print resources: 5%
    // Electronic resources: 10%
    AcquisitionManager acq(catalog, budget, 5, 10);
    std::cout << "\n=== Q6 Tax Configuration ===\n";
    std::cout << "Print tax rate: " << acq.printTaxPercent() << "%\n";
    std::cout << "Electronic tax rate: "
            << acq.electronicTaxPercent() << "%\n";

    std::cout << "B001, 1 copy pre-tax: "
            << Money::of(450)
            << ", post-tax quote: "
            << acq.quote("B001", 1) << "\n";

    std::cout << "E001, 1 seat pre-tax: 800.00, "
          << "post-tax quote " << acq.quote("E001", 1) << "\n";
    std::cout << "\n=== Quotes ===\n";
    std::cout << "5 copies of B002  = " << acq.quote("B002", 5) << "\n";
    std::cout << "2 copies of J001 for 1 year = " << acq.quote("J001", 2) << "\n";
    std::cout << "5 seats of E001 = " << acq.quote("E001", 5)<< "  (EBook)\n";
    std::cout << "3 seats of A001 = " << acq.quote("A001", 3) << "  (AudioBook)\n";
    std::cout << "1 copy of T001 = " << acq.quote("T001", 1) << "  (Thesis, usually free)\n";
    std::cout << "20 seats of R001  = " << acq.quote("R001", 20) << "  (incl. platform fee)\n";
    std::cout << "Q5 bulk discount quotes (now including Q6 tax):\n";
    std::cout << "  10 copies of B002 (hardcover + bulk discount): "
            << acq.quote("B002", 10) << "\n";
    std::cout << "  60 seats of R001 (first 50 full price, remaining half price): "
            << acq.quote("R001", 60) << "\n";

    acq.processBatch({
    {"B001", 4},   // 1800 + 5% tax = 1890, approved
    {"B002", 4},   // 5760 + 5% tax = 6048, approved
    {"B001", 1},   // 450 + 5% tax = 472.50, rejected: Book quota
    {"J001", 6},   // 600 + 5% tax = 630, approved
    {"R001", 20},  // 5000 + 10% tax = 5500, approved
    {"R002", 25},  // rejected: e-resource unit quota
    {"R002", 15},  // 6000 + 10% tax = 6600, approved
    {"R002", 5},   // 2000 + 10% tax = 2200, rejected: spend quota
    {"E001", 5},   // 1700 + 10% tax = 1870, approved
    {"A001", 3},   // 900 + 10% tax = 990, approved
    {"T001", 1},   // 0 + tax = 0, approved: Thesis is free
    {"X999", 1},   // rejected: unknown id
    });
    std::cout << "\n=== Acquisition report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";
    budget.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource* r : catalog.all())
        std::cout << "  " << r->id() << ": " << catalog.holdings(r->id())
                  << (r->isDigital() ? " seats" : " copies") << "\n";

    // Direct purchase: errors are reported with exceptions.
    // Q6 tax is included when checking the remaining quota.
    std::cout << "\n=== Direct purchase that breaks a quota ===\n";
    try {
        acq.purchase("B002", 1);
    } catch (const QuotaExceededError& e) {
        std::cout << "QuotaExceededError: " << e.what() << "\n";
    }
    return 0;
}
