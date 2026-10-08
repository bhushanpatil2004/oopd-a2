// Minimal self-contained test runner (no external framework needed).
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>


#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

template <typename Exception, typename Function>
static void assertThrows(Function&& function) {
    bool thrown = false;

    try {
        function();
    } catch (const Exception&) {
        thrown = true;
    } catch (...) {
    }

    assert(thrown);
}





static void testMoney() {
    assert(Money::of(12, 5).toString() == "12.05");
    assert(Money::of(-3, 50).toString() == "-3.50");
    assert(Money::fromMinor(7).toString() == "0.07");
    assert(Money::of(10) + Money::of(0, 50) ==
          Money::fromMinor(1050));
    assert(Money::of(3) * 4 == Money::of(12));
    assert(Money::of(1) < Money::of(2));
    assertThrows<std::invalid_argument>([&] {
        (void)(Money::of(1, 100));
    });
}

static void testMoneyCurrencies() {
    Money inr = Money::of(100, 50, "INR");
    Money usd = Money::of(100, 50, "USD");

    assert(inr.currency() == "INR");
    assert(usd.currency() == "USD");

    assert(Money::of(10) == Money::of(10, 0, "INR"));
    assert(Money::fromMinor(1250, "INR").toString() == "12.50");

    assert(Money::of(10, 25, "INR") +
              Money::of(5, 75, "INR") ==
          Money::of(16, 0, "INR"));

    assert(Money::of(10, 25, "INR") -
              Money::of(5, 25, "INR") ==
          Money::of(5, 0, "INR"));

    assertThrows<std::invalid_argument>([&] {
        (void)(Money::of(10, 0, "INR") +
            Money::of(10, 0, "USD"));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(Money::of(10, 0, "INR") -
            Money::of(10, 0, "USD"));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(Money::of(10, 0, "INR") ==
            Money::of(10, 0, "USD"));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(Money::of(10, 0, "INR") <
            Money::of(20, 0, "USD"));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(Money::of(10, 0, ""));
    });
}

static void testResourcesAndCost() {
    Book b(
        "B1",
        "T",
        {"A", "B", "C"},
        "isbn",
        "P",
        2020,
        Money::of(100));

    assert(b.category() == ResourceCategory::Book);
    assert(!b.isDigital());
    assert(b.costFor(3) == Money::of(300));
    assertThrows<std::invalid_argument>([&] {
        (void)(b.costFor(0));
    });
    assert(joinAuthors(b.authors()) == "A, B and C");

    Journal j(
        "J1",
        "ACM Computing Surveys",
        "0360-0300",
        4,
        "ACM",
        2026,
        Money::of(500),
        3);

    assert(j.category() == ResourceCategory::Journal);
    assert(!j.isDigital());
    assert(j.issn() == "0360-0300");
    assert(j.issuesPerYear() == 4);
    assert(j.subscriptionYears() == 3);

    // 500 per copy per year × 2 copies × 3 years.
    assert(j.costFor(2) == Money::of(3000));

    // Default subscription length is one year.
    Journal oneYear(
        "J2",
        "Nature",
        "0028-0836",
        52,
        "Springer",
        2026,
        Money::of(800));

    assert(oneYear.subscriptionYears() == 1);
    assert(oneYear.costFor(2) == Money::of(1600));

    assertThrows<std::invalid_argument>([&] {
        (void)(Journal(
            "J3",
            "Invalid Journal",
            "0000-0000",
            12,
            "Publisher",
            2026,
            Money::of(100),
            0));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(j.costFor(0));
    });

    EBook ebook(
        "E001",
        "Modern C++",
        {"Bhushan Patil", "A. Developer"},
        "978-1234567890",
        "Tech Press",
        2026,
        Money::of(300),
        "https://ebooks.example/modern-cpp",
        LicenseModel::AnnualSubscription,
        Money::of(500),
        FileFormat::EPUB,
        true);

    assert(ebook.category() == ResourceCategory::EBook);
    assert(ebook.isDigital());
    assert(ebook.isbn() == "978-1234567890");
    assert(ebook.format() == FileFormat::EPUB);
    assert(ebook.drmProtected());
    assert(ebook.authors().size() == 2);

    // Pricing must remain inherited from ElectronicResource.
    assert(ebook.costFor(4) == Money::of(1700));

    // Q4 — Hardcover books cost 20% more than listed price.
    Book paperback(
        "B-Q4-P",
        "Q4 Paperback",
        {"Author"},
        "ISBN-Q4-P",
        "Publisher",
        2026,
        Money::of(450),
        1,
        Binding::Paperback);

    Book hardcover(
        "B-Q4-H",
        "Q4 Hardcover",
        {"Author"},
        "ISBN-Q4-H",
        "Publisher",
        2026,
        Money::of(450),
        1,
        Binding::Hardcover);

    assert(paperback.costFor(1) == Money::of(450));
    assert(hardcover.costFor(1) == Money::of(540));
    assert(paperback.costFor(2) == Money::of(900));
    assert(hardcover.costFor(2) == Money::of(1080));

    // Q5 — Bulk discounts for print items.
    Book q5Paperback(
        "B-Q5-P",
        "Q5 Paperback",
        {"Author"},
        "ISBN-Q5-P",
        "Publisher",
        2026,
        Money::of(100),
        1,
        Binding::Paperback);

    Book q5Hardcover(
        "B-Q5-H",
        "Q5 Hardcover",
        {"Author"},
        "ISBN-Q5-H",
        "Publisher",
        2026,
        Money::of(100),
        1,
        Binding::Hardcover);

    assert(q5Paperback.costFor(9) == Money::of(900));
    assert(q5Paperback.costFor(10) == Money::of(1000));
    assert(q5Hardcover.costFor(9) == Money::of(1080));
    assert(q5Hardcover.costFor(10) == Money::of(1200));

    // Q5 — Bulk discount for Journal.
    Journal q5Journal(
        "J-Q5",
        "Q5 Journal",
        "Publisher",
        2026,
        "ISSN-Q5",
        12,
        Money::of(100),
        1);

    assert(q5Journal.costFor(9) == Money::of(900));
    assert(q5Journal.costFor(10) == Money::of(1000));

    // Q5 — Electronic resource: 50-seat boundary.
    ElectronicResource q5Electronic(
        "ER-Q5",
        "Q5 Electronic Resource",
        "Publisher",
        2026,
        Money::of(100),
        "https://example.com/q5",
        LicenseModel::AnnualSubscription,
        Money::of(500));

    assert(q5Electronic.costFor(50) == Money::of(5500));
    assert(q5Electronic.costFor(51) == Money::of(5550));
    assert(q5Electronic.costFor(60) == Money::of(6000));

    ElectronicResource e(
        "R1",
        "DB",
        "P",
        2026,
        Money::of(10),
        "url",
        LicenseModel::AnnualSubscription,
        Money::of(100));

    assert(e.isDigital());
    assert(e.costFor(5) == Money::of(150));
    assert(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference.
    const Resource& journalResource = j;

    assert(journalResource.category() ==
          ResourceCategory::Journal);
    assert(journalResource.costFor(2) == Money::of(3000));

    const ElectronicResource& electronic = ebook;

    assert(electronic.costFor(4) == Money::of(1700));
    assert(electronic.category() == ResourceCategory::EBook);

    const Resource& r = e;

    assert(r.costFor(1) == Money::of(110));

    std::ostringstream os;
    os << r;

    assert(os.str().find("platform fee: 100.00") !=
          std::string::npos);

    assertThrows<std::invalid_argument>([&] {
        (void)(Book("", "T", {}, "", "", 2000, Money::of(1)));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(Book(
            "B",
            "T",
            {},
            "",
            "",
            2000,
            Money::fromMinor(-1)));
    });
}

static void testCatalog() {
    Catalog c;

    c.emplace<Book>(
        "B1",
        "Clean Code",
        std::vector<std::string>{"M"},
        "i",
        "P",
        2008,
        Money::of(1));

    c.emplace<Book>(
        "B2",
        "Clean Architecture",
        std::vector<std::string>{"M"},
        "i",
        "P",
        2017,
        Money::of(1));

    c.emplace<ElectronicResource>(
        "R1",
        "ACM Digital Library",
        "ACM",
        2026,
        Money::of(1),
        "url");

    assert(c.size() == 3);
    assert(c.contains("B1"));
    assert(c.find("nope") == nullptr);
    assertThrows<NotFoundError>([&] {
        (void)(c.get("nope"));
    });

    assertThrows<DuplicateIdError>([&] {
        (void)(c.emplace<Book>(
            "B1",
            "dup",
            std::vector<std::string>{},
            "",
            "",
            1,
            Money::of(1)));
    });

    assert(c.searchTitle("clean").size() == 2);
    assert(c.byCategory(ResourceCategory::ElectronicResource)
              .size() == 1);

    assert(c.where([](const Resource& r) {
        return r.isDigital();
    }).size() == 1);

    assert(c.holdings("B1") == 0);

    c.addHoldings("B1", 3);

    assert(c.holdings("B1") == 3);
    assertThrows<std::invalid_argument>([&] {
        (void)(c.addHoldings("B1", -5));
    });

    c.remove("R1");

    assert(c.size() == 2);
    assertThrows<NotFoundError>([&] {
        (void)(c.remove("R1"));
    });
}

static void testBudget() {
    Budget b(Money::of(1000));

    b.setQuota(
        ResourceCategory::Book,
        {5, Money::of(400)});

    b.setQuota(
        ResourceCategory::Journal,
        {4, Money::of(3000)});

    assert(
        b.check(
             ResourceCategory::Journal,
             3,
             Money::of(800))
            .empty());

    assert(
        !b.check(
             ResourceCategory::Journal,
             5,
             Money::of(100))
             .empty());

    assert(
        !b.check(
             ResourceCategory::Journal,
             1,
             Money::of(3001))
             .empty());

    assert(
        b.check(
             ResourceCategory::Book,
             2,
             Money::of(200))
            .empty());

    assert(
        !b.check(
             ResourceCategory::Book,
             6,
             Money::of(10))
             .empty());

    assert(
        !b.check(
             ResourceCategory::Book,
             1,
             Money::of(401))
             .empty());

    assert(
        !b.check(
             ResourceCategory::ElectronicResource,
             1,
             Money::of(1001))
             .empty());

    assert(
        b.check(
             ResourceCategory::ElectronicResource,
             1,
             Money::of(900))
            .empty());

    b.commit(
        ResourceCategory::Book,
        4,
        Money::of(300));

    assert(b.spent() == Money::of(300));
    assert(*b.unitsRemaining(ResourceCategory::Book) == 1);
    assert(*b.spendRemaining(ResourceCategory::Book) ==
          Money::of(100));
    assert(
        !b.unitsRemaining(
             ResourceCategory::ElectronicResource)
             .has_value());

    assertThrows<QuotaExceededError>([&] {
        (void)(b.commit(
            ResourceCategory::Book,
            2,
            Money::of(10)));
    });

    assertThrows<BudgetExceededError>([&] {
        (void)(b.commit(
            ResourceCategory::ElectronicResource,
            1,
            Money::of(800)));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(b.commit(
            ResourceCategory::ElectronicResource,
            0,
            Money::of(1)));
    });

    assert(b.spent() == Money::of(300));

    Budget ebookBudget(Money::of(25000));

    ebookBudget.setQuota(
        ResourceCategory::EBook,
        {5, Money::of(2500)});

    assert(
        ebookBudget.check(
                       ResourceCategory::EBook,
                       3,
                       Money::of(1700))
            .empty());

    assert(
        !ebookBudget.check(
             ResourceCategory::EBook,
             6,
             Money::of(1700))
             .empty());

    assert(
        !ebookBudget.check(
             ResourceCategory::EBook,
             2,
             Money::of(2600))
             .empty());

    // Q3: AudioBook.
    AudioBook audiobook(
        "A1",
        "C++ Audio Guide",
        "Jane Smith",
        360,
        "Audio Press",
        2026,
        Money::of(200),
        "https://audio.example/cpp",
        LicenseModel::AnnualSubscription,
        Money::of(300));

    assert(audiobook.category() ==
          ResourceCategory::AudioBook);
    assert(audiobook.isDigital());
    assert(audiobook.narrator() == "Jane Smith");
    assert(audiobook.durationMinutes() == 360);
    assert(audiobook.costFor(4) == Money::of(1100));

    const ElectronicResource& audioElectronic = audiobook;

    assert(audioElectronic.costFor(4) == Money::of(1100));

    assertThrows<std::invalid_argument>([&] {
        (void)(AudioBook(
            "A2",
            "Invalid Audio",
            "Jane Smith",
            0,
            "Audio Press",
            2026,
            Money::of(200),
            "https://audio.example/invalid"));
    });

    // Q3: Thesis.
    Thesis thesis(
        "T1",
        "Efficient Algorithms",
        "ABC University",
        "M.Tech",
        "Dr. Rao",
        2026);

    assert(thesis.category() == ResourceCategory::Thesis);
    assert(!thesis.isDigital());
    assert(thesis.university() == "ABC University");
    assert(thesis.degree() == "M.Tech");
    assert(thesis.supervisor() == "Dr. Rao");
    assert(thesis.costFor(1) == Money{});
    assert(thesis.costFor(5) == Money{});

    Resource& thesisResource = thesis;

    assert(thesisResource.category() ==
          ResourceCategory::Thesis);
    assert(thesisResource.costFor(2) == Money{});
}

static void testAcquisition() {
    Catalog c;

    c.emplace<Book>(
        "B1",
        "Book",
        std::vector<std::string>{"A"},
        "i",
        "P",
        2020,
        Money::of(100));

    c.emplace<ElectronicResource>(
        "R1",
        "DB",
        "P",
        2026,
        Money::of(10),
        "url",
        LicenseModel::AnnualSubscription,
        Money::of(50));

    Budget b(Money::of(500));

    b.setQuota(
        ResourceCategory::Book,
        {3, Money::of(1000)});

    AcquisitionManager acq(c, b);

    assert(acq.quote("R1", 5) == Money::of(100));

    std::string why;

    assert(
        acq.canPurchase("B1", 3, &why) &&
        why.empty());

    assert(
        !acq.canPurchase("B1", 4, &why) &&
        !why.empty());

    assert(!acq.canPurchase("nope", 1, &why));

    const auto& rec =
        acq.purchase("B1", 2);

    assert(
        rec.approved &&
        rec.cost == Money::of(200) &&
        rec.orderNo == 1);

    assert(c.holdings("B1") == 2);

    assertThrows<QuotaExceededError>([&] {
        (void)(acq.purchase("B1", 2));
    });

    assertThrows<NotFoundError>([&] {
        (void)(acq.purchase("nope", 1));
    });

    assert(acq.history().size() == 1);

    auto res = acq.processBatch({
        {"Default", "R1", 10},
        {"Default", "R1", 100},
        {"Default", "B1", 1},
        {"Default", "zzz", 1},
        {"Default", "B1", 0}
    });

    assert(res.size() == 5);
    assert(
        res[0].approved &&
        res[0].cost == Money::of(150));
    assert(!res[1].approved);
    assert(res[2].approved);

    assert(
        !res[3].approved &&
        res[3].reason.find("not found") !=
            std::string::npos);

    assert(!res[4].approved);

    assert(acq.totalSpent() == Money::of(450));
    assert(b.spent() == acq.totalSpent());
    assert(
        c.holdings("R1") == 10 &&
        c.holdings("B1") == 3);
    assert(acq.history().size() == 6);
}

static void testTaxes() {
    Catalog c;

    c.emplace<Book>(
        "Q6-B",
        "Tax Book",
        std::vector<std::string>{"Author"},
        "ISBN-Q6",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<ElectronicResource>(
        "Q6-E",
        "Tax Electronic",
        "Publisher",
        2026,
        Money::of(100),
        "url",
        LicenseModel::AnnualSubscription,
        Money::of(0));

    Budget b(Money::of(1000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(150)});

    b.setQuota(
        ResourceCategory::ElectronicResource,
        {10, Money::of(150)});

    AcquisitionManager acq(c, b, 10, 20);

    assert(acq.printTaxPercent() == 10);
    assert(acq.electronicTaxPercent() == 20);

    assert(acq.quote("Q6-B", 1) == Money::of(110));
    assert(acq.quote("Q6-E", 1) == Money::of(120));

    const auto& bookRecord =
        acq.purchase("Q6-B", 1);

    assert(bookRecord.preTaxCost == Money::of(100));
    assert(bookRecord.tax == Money::of(10));
    assert(bookRecord.cost == Money::of(110));

    assert(b.spent() == Money::of(110));

    assert(
        b.usageFor(ResourceCategory::Book).spent ==
        Money::of(110));

    acq.setTaxRates(5, 15);

    assert(acq.printTaxPercent() == 5);
    assert(acq.electronicTaxPercent() == 15);

    assert(acq.quote("Q6-B", 1) == Money::of(105));
    assert(acq.quote("Q6-E", 1) == Money::of(115));

    assertThrows<std::invalid_argument>([&] {
        (void)(acq.setTaxRates(-1, 10));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(acq.setTaxRates(10, 101));
    });
}

static void testPostTaxQuota() {
    Catalog c;

    c.emplace<Book>(
        "Q6-Q",
        "Quota Tax Book",
        std::vector<std::string>{"Author"},
        "ISBN-Q6-Q",
        "Publisher",
        2026,
        Money::of(100));

    Budget b(Money::of(5000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(105)});

    AcquisitionManager acq(c, b, 10, 0);

    assert(acq.quote("Q6-Q", 1) == Money::of(110));

    std::string reason;

    assert(
        !acq.canPurchase(
            "Q6-Q",
            1,
            &reason));

    assert(!reason.empty());
    assert(b.spent() == Money{});
}

static void testTitleLimits() {
    Catalog c;

    c.emplace<Book>(
        "Q7-B1",
        "First Book",
        std::vector<std::string>{"Author One"},
        "ISBN-Q7-1",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Book>(
        "Q7-B2",
        "Second Book",
        std::vector<std::string>{"Author Two"},
        "ISBN-Q7-2",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Book>(
        "Q7-B3",
        "Third Book",
        std::vector<std::string>{"Author Three"},
        "ISBN-Q7-3",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Journal>(
        "Q7-J1",
        "First Journal",
        "ISSN-Q7-1",
        12,
        "Publisher",
        2026,
        Money::of(50),
        1);

    Budget b(Money::of(5000));

    b.setTitleLimit(
        ResourceCategory::Book,
        2);

    assert(
        b.titleLimitFor(
            ResourceCategory::Book)
            .has_value());

    assert(
        *b.titleLimitFor(
            ResourceCategory::Book) == 2);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 0);

    assert(
        b.checkTitle(
            ResourceCategory::Book,
            "Q7-B1")
            .empty());

    b.commitTitle(
        ResourceCategory::Book,
        "Q7-B1");

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    assert(
        b.checkTitle(
            ResourceCategory::Book,
            "Q7-B1")
            .empty());

    b.commitTitle(
        ResourceCategory::Book,
        "Q7-B1");

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    assert(
        b.checkTitle(
            ResourceCategory::Book,
            "Q7-B2")
            .empty());

    b.commitTitle(
        ResourceCategory::Book,
        "Q7-B2");

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    assert(
        !b.checkTitle(
             ResourceCategory::Book,
             "Q7-B3")
             .empty());

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    assert(
        !b.titleLimitFor(
             ResourceCategory::Journal)
             .has_value());

    assert(
        b.checkTitle(
            ResourceCategory::Journal,
            "Q7-J1")
            .empty());

    Budget zeroTitleBudget(Money::of(1000));

    zeroTitleBudget.setTitleLimit(
        ResourceCategory::Book,
        0);

    assert(
        !zeroTitleBudget.checkTitle(
             ResourceCategory::Book,
             "Q7-B1")
             .empty());

    assert(
        zeroTitleBudget.titlesUsed(
            ResourceCategory::Book) == 0);

    assertThrows<std::invalid_argument>([&] {
        (void)(b.setTitleLimit(
            ResourceCategory::Book,
            -1));
    });
}

static void testAcquisitionTitleLimits() {
    Catalog c;

    c.emplace<Book>(
        "Q7-B1",
        "First Book",
        std::vector<std::string>{"Author One"},
        "ISBN-Q7-1",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Book>(
        "Q7-B2",
        "Second Book",
        std::vector<std::string>{"Author Two"},
        "ISBN-Q7-2",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Book>(
        "Q7-B3",
        "Third Book",
        std::vector<std::string>{"Author Three"},
        "ISBN-Q7-3",
        "Publisher",
        2026,
        Money::of(100));

    Budget b(Money::of(1000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000)});

    b.setTitleLimit(
        ResourceCategory::Book,
        2);

    AcquisitionManager acq(c, b);

    std::string reason;

    assert(
        acq.canPurchase(
            "Q7-B1",
            1,
            &reason));

    assert(reason.empty());

    const auto& first =
        acq.purchase("Q7-B1", 1);

    assert(first.approved);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    assert(
        acq.canPurchase(
            "Q7-B1",
            2,
            &reason));

    assert(reason.empty());

    const auto& sameTitle =
        acq.purchase("Q7-B1", 2);

    assert(sameTitle.approved);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    assert(
        acq.canPurchase(
            "Q7-B2",
            1,
            &reason));

    assert(reason.empty());

    const auto& second =
        acq.purchase("Q7-B2", 1);

    assert(second.approved);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    assert(
        !acq.canPurchase(
             "Q7-B3",
             1,
             &reason));

    assert(!reason.empty());

    assert(
        reason.find("title") !=
        std::string::npos);

    assertThrows<QuotaExceededError>([&] {
        (void)(acq.purchase("Q7-B3", 1));
    });

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    assert(c.holdings("Q7-B3") == 0);

    auto results = acq.processBatch({
        {"Default", "Q7-B3", 1},
        {"Default", "Q7-B1", 1}
    });

    assert(results.size() == 2);
    assert(!results[0].approved);

    assert(
        results[0].reason.find("title") !=
        std::string::npos);

    assert(results[1].approved);

    assert(c.holdings("Q7-B1") == 4);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);
}

/*
 * Q8 — Cancellation
 */
static void testCancellation() {
    Catalog c;

    c.emplace<Book>(
        "Q8-B1",
        "Cancellation Book One",
        std::vector<std::string>{"Author One"},
        "ISBN-Q8-1",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Book>(
        "Q8-B2",
        "Cancellation Book Two",
        std::vector<std::string>{"Author Two"},
        "ISBN-Q8-2",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Book>(
        "Q8-B3",
        "Cancellation Book Three",
        std::vector<std::string>{"Author Three"},
        "ISBN-Q8-3",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Book>(
        "Q8-B4",
        "Cancellation Book Four",
        std::vector<std::string>{"Author Four"},
        "ISBN-Q8-4",
        "Publisher",
        2026,
        Money::of(100));

    Budget b(Money::of(5000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(5000)});

    b.setTitleLimit(
        ResourceCategory::Book,
        2);

    AcquisitionManager acq(c, b, 10, 0);

    // Two separate purchases of the same title.
    PurchaseRecord first =
        acq.purchase("Q8-B1", 2);

    PurchaseRecord second =
        acq.purchase("Q8-B1", 1);

    PurchaseRecord other =
        acq.purchase("Q8-B2", 1);

    assert(first.approved && !first.cancellation);
    assert(second.approved && !second.cancellation);
    assert(other.approved && !other.cancellation);

    assert(b.spent() == Money::of(440));

    assert(
        b.usageFor(
             ResourceCategory::Book).units == 4);

    assert(
        b.usageFor(
             ResourceCategory::Book).spent ==
        Money::of(440));

    assert(c.holdings("Q8-B1") == 3);
    assert(c.holdings("Q8-B2") == 1);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    assert(acq.totalSpent() == Money::of(440));

    // Cancelling one of two purchases of B1 must not release
    // its title slot.
    PurchaseRecord cancellation1 =
        acq.cancel(first.orderNo);

    assert(cancellation1.cancellation);
    assert(cancellation1.approved);
    assert(cancellation1.resourceId == "Q8-B1");
    assert(cancellation1.quantity == 2);
    assert(cancellation1.cost == Money{});

    assert(
        cancellation1.reason.find("cancelled order") !=
        std::string::npos);

    assert(b.spent() == Money::of(220));

    assert(
        b.usageFor(
             ResourceCategory::Book).units == 2);

    assert(
        b.usageFor(
             ResourceCategory::Book).spent ==
        Money::of(220));

    assert(c.holdings("Q8-B1") == 1);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    assert(acq.totalSpent() == Money::of(220));
    assert(acq.history().size() == 4);

    // The original purchase remains in history.
    assert(
        acq.history()[0].orderNo ==
        first.orderNo);

    assert(acq.history()[0].approved);
    assert(!acq.history()[0].cancellation);

    // Cancelling the last active B1 purchase releases its title slot.
    PurchaseRecord cancellation2 =
        acq.cancel(second.orderNo);

    assert(cancellation2.cancellation);

    assert(b.spent() == Money::of(110));

    assert(
        b.usageFor(
             ResourceCategory::Book).units == 1);

    assert(
        b.usageFor(
             ResourceCategory::Book).spent ==
        Money::of(110));

    assert(c.holdings("Q8-B1") == 0);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    assert(acq.totalSpent() == Money::of(110));

    // B3 can now use the title slot released by B1.
    assert(acq.canPurchase("Q8-B3", 1));

    const auto& third =
        acq.purchase("Q8-B3", 1);

    assert(third.approved);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    assert(c.holdings("Q8-B3") == 1);

    // A third different active title is rejected by Q7.
    auto rejected =
        acq.processBatch({
            {"Default", "Q8-B4", 1}
        });

    assert(rejected.size() == 1);
    assert(!rejected[0].approved);
    assert(!rejected[0].cancellation);

    assert(
        rejected[0].reason.find("title") !=
        std::string::npos);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    assert(c.holdings("Q8-B4") == 0);

    // A rejected order cannot be cancelled.
    assertThrows<std::invalid_argument>([&] {
        (void)(acq.cancel(rejected[0].orderNo));
    });

    // An already cancelled order cannot be cancelled again.
    assertThrows<std::invalid_argument>([&] {
        (void)(acq.cancel(first.orderNo));
    });

    // A cancellation record cannot itself be cancelled.
    assertThrows<std::invalid_argument>([&] {
        (void)(acq.cancel(cancellation1.orderNo));
    });

    // Unknown order numbers are rejected.
    assertThrows<NotFoundError>([&] {
        (void)(acq.cancel(999999));
    });

    // Cancel the remaining active titles and verify
    // all Book usage is refunded.
    acq.cancel(other.orderNo);
    acq.cancel(third.orderNo);

    assert(b.spent() == Money{});

    assert(
        b.usageFor(
             ResourceCategory::Book).units == 0);

    assert(
        b.usageFor(
             ResourceCategory::Book).spent ==
        Money{});

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 0);

    assert(c.holdings("Q8-B1") == 0);
    assert(c.holdings("Q8-B2") == 0);
    assert(c.holdings("Q8-B3") == 0);

    assert(acq.totalSpent() == Money{});
}

/*
 * Q9 — Department budgets
 */
/*
 * Q9 — Department budgets
 */
static void testDepartmentBudgets() {
    Catalog c;

    c.emplace<Book>(
        "Q9-B1",
        "Department Book One",
        std::vector<std::string>{"Author One"},
        "ISBN-Q9-1",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Book>(
        "Q9-B2",
        "Department Book Two",
        std::vector<std::string>{"Author Two"},
        "ISBN-Q9-2",
        "Publisher",
        2026,
        Money::of(200));

    c.emplace<Book>(
        "Q9-B3",
        "Department Book Three",
        std::vector<std::string>{"Author Three"},
        "ISBN-Q9-3",
        "Publisher",
        2026,
        Money::of(300));

    Budget defaultBudget(Money::of(1000));

    defaultBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000)});

    Budget csBudget(Money::of(1000));

    csBudget.setQuota(
        ResourceCategory::Book,
        {5, Money::of(1000)});

    // Computer Science can buy three different Book titles.
    csBudget.setTitleLimit(
        ResourceCategory::Book,
        3);

    Budget physicsBudget(Money::of(1000));

    physicsBudget.setQuota(
        ResourceCategory::Book,
        {1, Money::of(150)});

    // Physics can buy only one different Book title.
    physicsBudget.setTitleLimit(
        ResourceCategory::Book,
        1);

    AcquisitionManager acq(
        c,
        defaultBudget);

    acq.addDepartment(
        "Computer Science",
        csBudget);

    acq.addDepartment(
        "Physics",
        physicsBudget);

    assert(
        &acq.departmentBudget("Computer Science") ==
        &csBudget);

    assert(
        &acq.departmentBudget("Physics") ==
        &physicsBudget);

    assert(
        &acq.departmentBudget("Default") ==
        &defaultBudget);

    assertThrows<NotFoundError>([&] {
        (void)(acq.departmentBudget("Mathematics"));
    });

    // ---------------------------------------------------------
    // Same resource, different departments
    // ---------------------------------------------------------

    assert(
        acq.quote(
            "Computer Science",
            "Q9-B1",
            2) ==
        Money::of(200));

    assert(
        acq.canPurchase(
            "Computer Science",
            "Q9-B1",
            2));

    // Store copies, not references. The acquisition history vector
    // may reallocate when another purchase is added.
    PurchaseRecord csFirst =
        acq.purchase(
            "Computer Science",
            "Q9-B1",
            2);

    assert(csFirst.approved);
    assert(csFirst.department == "Computer Science");
    assert(csBudget.spent() == Money::of(200));

    assert(
        csBudget.usageFor(
                    ResourceCategory::Book)
            .units == 2);

    assert(c.holdings("Q9-B1") == 2);

    // The same title has an independent title slot in Physics.
    PurchaseRecord physicsFirst =
        acq.purchase(
            "Physics",
            "Q9-B1",
            1);

    assert(physicsFirst.approved);
    assert(physicsFirst.department == "Physics");
    assert(physicsBudget.spent() == Money::of(100));

    assert(
        physicsBudget.usageFor(
                        ResourceCategory::Book)
            .units == 1);

    assert(
        physicsBudget.titlesUsed(
            ResourceCategory::Book) == 1);

    assert(
        csBudget.titlesUsed(
            ResourceCategory::Book) == 1);

    // ---------------------------------------------------------
    // Physics has reached its unit quota.
    // Computer Science still has available quota.
    // ---------------------------------------------------------

    std::string reason;

    assert(
        !acq.canPurchase(
             "Physics",
             "Q9-B2",
             1,
             &reason));

    assert(!reason.empty());

    assert(
        acq.canPurchase(
            "Computer Science",
            "Q9-B2",
            1,
            &reason));

    PurchaseRecord csSecond =
        acq.purchase(
            "Computer Science",
            "Q9-B2",
            1);

    assert(csSecond.approved);
    assert(csBudget.spent() == Money::of(400));
    assert(physicsBudget.spent() == Money::of(100));

    // Physics has a one-title limit, so a new title is rejected.
    assert(
        !acq.canPurchase(
             "Physics",
             "Q9-B2",
             1,
             &reason));

    assert(
        reason.find("title") !=
        std::string::npos);

    // Computer Science has a separate title limit of three.
    assert(
        acq.canPurchase(
            "Computer Science",
            "Q9-B3",
            1,
            &reason));

    // ---------------------------------------------------------
    // Batch requests are department-specific.
    // ---------------------------------------------------------

    auto batch =
        acq.processBatch({
            {"Computer Science", "Q9-B3", 1},
            {"Physics", "Q9-B3", 1},
            {"Mathematics", "Q9-B3", 1}
        });

    assert(batch.size() == 3);

    // CS accepts its third different title.
    assert(batch[0].approved);
    assert(
        batch[0].department ==
        "Computer Science");

    // Physics rejects because its title limit is already full.
    assert(!batch[1].approved);
    assert(
        batch[1].department ==
        "Physics");

    assert(
        batch[1].reason.find("title") !=
        std::string::npos);

    // Mathematics has no registered department budget.
    assert(!batch[2].approved);

    assert(
        batch[2].reason.find("department not found") !=
        std::string::npos);

    assert(csBudget.spent() == Money::of(700));
    assert(physicsBudget.spent() == Money::of(100));
    assert(c.holdings("Q9-B3") == 1);

    assert(
        csBudget.titlesUsed(
            ResourceCategory::Book) == 3);

    assert(
        physicsBudget.titlesUsed(
            ResourceCategory::Book) == 1);

    // ---------------------------------------------------------
    // Q8 + Q9:
    // Cancellation must refund the correct department budget.
    // ---------------------------------------------------------

    const int csSecondOrderNo =
        csSecond.orderNo;

    const int csSecondQuantity =
        csSecond.quantity;

    assert(csSecondQuantity == 1);

    PurchaseRecord cancelled =
        acq.cancel(csSecondOrderNo);

    assert(cancelled.cancellation);
    assert(
        cancelled.department ==
        "Computer Science");

    // B1 (200) + B3 (300) remain active.
    // B2 (200) was cancelled.
    assert(csBudget.spent() == Money::of(500));

    assert(
        csBudget.usageFor(
                    ResourceCategory::Book)
            .units == 3);

    // B2 holdings were reduced by the cancellation.
    assert(c.holdings("Q9-B2") == 0);

    // Physics remains completely independent.
    assert(physicsBudget.spent() == Money::of(100));
    assert(
        physicsBudget.usageFor(
                    ResourceCategory::Book)
            .units == 1);

    // The Computer Science B2 title slot was released.
    assert(
        csBudget.titlesUsed(
            ResourceCategory::Book) == 2);

    // Physics still has its own B1 title slot.
    assert(
        physicsBudget.titlesUsed(
            ResourceCategory::Book) == 1);

    // ---------------------------------------------------------
    // Department information is retained in history.
    // ---------------------------------------------------------

    assert(
        acq.history()[csFirst.orderNo - 1].department ==
        "Computer Science");

    assert(
        acq.history().back().cancellation);

    assert(
        acq.history().back().department ==
        "Computer Science");
}

/*
 * Q10 — Year-end budget rollover
 */
static void testBudgetRollover() {
    Budget current(Money::of(1000));

    current.setQuota(
        ResourceCategory::Book,
        {10, Money::of(800)});

    current.setTitleLimit(
        ResourceCategory::Book,
        2);

    // Spend 300, leaving 700 unspent.
    current.commit(
        ResourceCategory::Book,
        3,
        Money::of(300));

    current.commitTitle(
        ResourceCategory::Book,
        "Q10-B1");

    assert(current.total() == Money::of(1000));
    assert(current.spent() == Money::of(300));
    assert(current.remaining() == Money::of(700));

    assert(
        current.usageFor(ResourceCategory::Book).units == 3);

    assert(
        current.usageFor(ResourceCategory::Book).spent ==
        Money::of(300));

    assert(
        current.titlesUsed(ResourceCategory::Book) == 1);

    // 50% of the unspent 700 = 350.
    Budget nextYear = current.rollover(50);

    assert(nextYear.total() == Money::of(350));
    assert(nextYear.spent() == Money{});
    assert(nextYear.remaining() == Money::of(350));

    // Category quota configuration is carried forward.
    assert(
        nextYear.quotaFor(ResourceCategory::Book).has_value());

    assert(
        nextYear.quotaFor(ResourceCategory::Book)->maxUnits == 10);

    assert(
        nextYear.quotaFor(ResourceCategory::Book)->maxSpend ==
        Money::of(800));

    // Q7 title-limit configuration is carried forward.
    assert(
        nextYear.titleLimitFor(ResourceCategory::Book).has_value());

    assert(
        *nextYear.titleLimitFor(ResourceCategory::Book) == 2);

    // Actual usage starts from zero in the new year.
    assert(
        nextYear.usageFor(ResourceCategory::Book).units == 0);

    assert(
        nextYear.usageFor(ResourceCategory::Book).spent ==
        Money{});

    // Purchased-title history does not carry into the new year.
    assert(
        nextYear.titlesUsed(ResourceCategory::Book) == 0);

    // The original budget is unchanged.
    assert(current.total() == Money::of(1000));
    assert(current.spent() == Money::of(300));
    assert(current.remaining() == Money::of(700));

    assert(
        current.usageFor(ResourceCategory::Book).units == 3);

    assert(
        current.titlesUsed(ResourceCategory::Book) == 1);

    // 0% rollover creates a zero-value new budget.
    Budget zeroRollover = current.rollover(0);

    assert(zeroRollover.total() == Money{});
    assert(zeroRollover.spent() == Money{});
    assert(zeroRollover.remaining() == Money{});

    // 100% rollover carries the complete unspent amount.
    Budget fullRollover = current.rollover(100);

    assert(fullRollover.total() == Money::of(700));
    assert(fullRollover.spent() == Money{});
    assert(fullRollover.remaining() == Money::of(700));

    // Invalid rollover percentages are rejected.
    assertThrows<std::invalid_argument>([&] {
        (void)(current.rollover(-1));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(current.rollover(101));
    });
}

/*
 * Q11 — All-or-nothing batch processing
 */
static void testAllOrNothingBatch() {
    Catalog c;

    c.emplace<Book>(
        "Q11-B1",
        "First Batch Book",
        std::vector<std::string>{"Author One"},
        "ISBN-Q11-1",
        "Publisher",
        2026,
        Money::of(100));

    c.emplace<Book>(
        "Q11-B2",
        "Second Batch Book",
        std::vector<std::string>{"Author Two"},
        "ISBN-Q11-2",
        "Publisher",
        2026,
        Money::of(200));

    c.emplace<Book>(
        "Q11-B3",
        "Third Batch Book",
        std::vector<std::string>{"Author Three"},
        "ISBN-Q11-3",
        "Publisher",
        2026,
        Money::of(300));

    Budget b(Money::of(2000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1500)});

    b.setTitleLimit(
        ResourceCategory::Book,
        3);

    AcquisitionManager acq(c, b);

    // ---------------------------------------------------------
    // Q11 — Successful all-or-nothing batch
    // ---------------------------------------------------------

    auto successfulBatch = acq.processBatch(
        {
            {"Default", "Q11-B1", 1},
            {"Default", "Q11-B2", 1}
        },
        true);

    assert(successfulBatch.size() == 2);

    assert(successfulBatch[0].approved);
    assert(successfulBatch[1].approved);

    assert(
        b.spent() == Money::of(300));

    assert(
        b.usageFor(
            ResourceCategory::Book).units == 2);

    assert(
        b.usageFor(
            ResourceCategory::Book).spent ==
        Money::of(300));

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    assert(c.holdings("Q11-B1") == 1);
    assert(c.holdings("Q11-B2") == 1);

    assert(acq.history().size() == 2);

    // ---------------------------------------------------------
    // Q11 — Failed all-or-nothing batch
    // ---------------------------------------------------------
    //
    // B3 is valid, but Q11-UNKNOWN does not exist.
    // Therefore NOTHING from this batch should be bought.
    //

    const Money spentBefore =
        b.spent();

    const int unitsBefore =
        b.usageFor(
            ResourceCategory::Book).units;

    const Money categorySpentBefore =
        b.usageFor(
            ResourceCategory::Book).spent;

    const int titlesBefore =
        b.titlesUsed(
            ResourceCategory::Book);

    const int b1HoldingsBefore =
        c.holdings("Q11-B1");

    const int b2HoldingsBefore =
        c.holdings("Q11-B2");

    const int b3HoldingsBefore =
        c.holdings("Q11-B3");

    const std::size_t historyBefore =
        acq.history().size();

    auto failedBatch = acq.processBatch(
        {
            {"Default", "Q11-B3", 1},
            {"Default", "Q11-UNKNOWN", 1}
        },
        true);

    // The entire batch is rejected.
    assert(failedBatch.empty());

    // Budget must be completely unchanged.
    assert(b.spent() == spentBefore);

    assert(
        b.usageFor(
            ResourceCategory::Book).units ==
        unitsBefore);

    assert(
        b.usageFor(
            ResourceCategory::Book).spent ==
        categorySpentBefore);

    // The new title must NOT consume a title slot.
    assert(
        b.titlesUsed(
            ResourceCategory::Book) ==
        titlesBefore);

    // Holdings must be completely unchanged.
    assert(
        c.holdings("Q11-B1") ==
        b1HoldingsBefore);

    assert(
        c.holdings("Q11-B2") ==
        b2HoldingsBefore);

    assert(
        c.holdings("Q11-B3") ==
        b3HoldingsBefore);

    // No purchase records should be created.
    assert(
        acq.history().size() ==
        historyBefore);

    // ---------------------------------------------------------
    // Q11 — Normal batch behaviour is unchanged
    // ---------------------------------------------------------
    //
    // Without the all-or-nothing option, requests are processed
    // independently, as in Q1-Q10.
    //

    auto normalBatch = acq.processBatch(
        {
            {"Default", "Q11-B3", 1},
            {"Default", "Q11-UNKNOWN", 1}
        });

    assert(normalBatch.size() == 2);

    assert(normalBatch[0].approved);

    assert(!normalBatch[1].approved);

    assert(
        normalBatch[1].reason.find("not found") !=
        std::string::npos);

    // The valid request remains purchased.
    assert(c.holdings("Q11-B3") == 1);

    assert(
        b.spent() ==
        Money::of(600));

    assert(
        b.usageFor(
            ResourceCategory::Book).units == 3);

    assert(
        b.titlesUsed(
            ResourceCategory::Book) == 3);

    // Only the normal batch created two history records.
    assert(acq.history().size() == 4);
}

/*
 * Q12 — Vendors
 *
 * The same resource can be offered by multiple vendors at different
 * prices. The cheapest vendor must be selected automatically.
 */
static void testVendors() {
    Catalog c;

    c.emplace<Book>(
        "Q12-B1",
        "Vendor Test Book",
        std::vector<std::string>{"Vendor Author"},
        "ISBN-Q12-1",
        "Vendor Publisher",
        2026,
        Money::of(500));

    c.emplace<Book>(
        "Q12-B2",
        "Second Vendor Book",
        std::vector<std::string>{"Another Author"},
        "ISBN-Q12-2",
        "Vendor Publisher",
        2026,
        Money::of(800));

    Budget b(Money::of(5000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(5000)});

    b.setTitleLimit(
        ResourceCategory::Book,
        3);

    AcquisitionManager acq(c, b);

    // ---------------------------------------------------------
    // Q12 — Register multiple vendors for the same title
    // ---------------------------------------------------------

    acq.addVendor(
        "Q12-B1",
        "Vendor A",
        Money::of(500));

    acq.addVendor(
        "Q12-B1",
        "Vendor B",
        Money::of(420));

    acq.addVendor(
        "Q12-B1",
        "Vendor C",
        Money::of(470));

    // The cheapest vendor must be Vendor B at ₹420.
    auto cheapest = acq.cheapestVendor("Q12-B1");

    assert(cheapest == "Vendor B");

    // ---------------------------------------------------------
    // Q12 — A more expensive vendor must not be selected
    // ---------------------------------------------------------

    acq.addVendor(
        "Q12-B1",
        "Vendor D",
        Money::of(600));

    cheapest = acq.cheapestVendor("Q12-B1");

    assert(cheapest == "Vendor B");

    // ---------------------------------------------------------
    // Q12 — Quote must use the cheapest vendor
    // ---------------------------------------------------------

    assert(
        acq.quote("Q12-B1", 1) ==
        Money::of(420));

    assert(
        acq.quote("Q12-B1", 2) ==
        Money::of(840));

    // ---------------------------------------------------------
    // Q12 — Purchase must use the cheapest vendor
    // ---------------------------------------------------------

    PurchaseRecord first =
        acq.purchase("Q12-B1", 2);

    assert(first.approved);

    assert(first.vendor == "Vendor B");

    assert(first.preTaxCost == Money::of(840));

    assert(first.cost == Money::of(840));

    assert(b.spent() == Money::of(840));

    assert(c.holdings("Q12-B1") == 2);

    // ---------------------------------------------------------
    // Q12 — Adding a cheaper vendor changes future purchases
    // ---------------------------------------------------------

    acq.addVendor(
        "Q12-B1",
        "Vendor E",
        Money::of(390));

    cheapest = acq.cheapestVendor("Q12-B1");

    assert(cheapest == "Vendor E");

    assert(
        acq.quote("Q12-B1", 1) ==
        Money::of(390));

    PurchaseRecord second =
        acq.purchase("Q12-B1", 1);

    assert(second.approved);

    assert(second.vendor == "Vendor E");

    assert(second.preTaxCost == Money::of(390));

    assert(second.cost == Money::of(390));

    assert(b.spent() == Money::of(1230));

    assert(c.holdings("Q12-B1") == 3);

    // ---------------------------------------------------------
    // Q12 — A different title can have different vendors
    // ---------------------------------------------------------

    acq.addVendor(
        "Q12-B2",
        "Book Supplier",
        Money::of(800));

    acq.addVendor(
        "Q12-B2",
        "Academic Supplier",
        Money::of(750));

    cheapest = acq.cheapestVendor("Q12-B2");

    assert(cheapest == "Academic Supplier");

    assert(
        acq.quote("Q12-B2", 2) ==
        Money::of(1500));

    PurchaseRecord third =
        acq.purchase("Q12-B2", 2);

    assert(third.approved);

    assert(third.vendor == "Academic Supplier");

    assert(third.preTaxCost == Money::of(1500));

    assert(third.cost == Money::of(1500));

    assert(c.holdings("Q12-B2") == 2);

    assert(b.spent() == Money::of(2730));

    // ---------------------------------------------------------
    // Q12 — Batch purchases must also use cheapest vendors
    // ---------------------------------------------------------

    auto batch = acq.processBatch({
        {"Default", "Q12-B1", 1},
        {"Default", "Q12-B2", 1}
    });

    assert(batch.size() == 2);

    assert(batch[0].approved);
    assert(batch[1].approved);

    assert(batch[0].vendor == "Vendor E");
    assert(batch[1].vendor == "Academic Supplier");

    assert(
        batch[0].preTaxCost ==
        Money::of(390));

    assert(
        batch[1].preTaxCost ==
        Money::of(750));

    assert(c.holdings("Q12-B1") == 4);
    assert(c.holdings("Q12-B2") == 3);

    assert(b.spent() == Money::of(3870));

    // ---------------------------------------------------------
    // Q12 — Vendor information must remain in order history
    // ---------------------------------------------------------

    assert(acq.history().size() == 5);

    assert(
        acq.history()[0].vendor ==
        "Vendor B");

    assert(
        acq.history()[1].vendor ==
        "Vendor E");

    assert(
        acq.history()[2].vendor ==
        "Academic Supplier");

    assert(
        acq.history()[3].vendor ==
        "Vendor E");

    assert(
        acq.history()[4].vendor ==
        "Academic Supplier");

    // ---------------------------------------------------------
    // Q12 — A resource without vendors keeps normal pricing
    // ---------------------------------------------------------

    c.emplace<Book>(
        "Q12-B3",
        "No Vendor Book",
        std::vector<std::string>{"Author"},
        "ISBN-Q12-3",
        "Publisher",
        2026,
        Money::of(250));

    assert(
        acq.quote("Q12-B3", 1) ==
        Money::of(250));

    PurchaseRecord noVendor =
        acq.purchase("Q12-B3", 1);

    assert(noVendor.approved);

    assert(noVendor.vendor.empty());

    assert(noVendor.cost == Money::of(250));

    assert(c.holdings("Q12-B3") == 1);

    // ---------------------------------------------------------
    // Q12 — Invalid vendor prices must be rejected
    // ---------------------------------------------------------

    assertThrows<std::invalid_argument>([&] {
        (void)(acq.addVendor(
            "Q12-B1",
            "Invalid Vendor",
            Money::fromMinor(-1)));
    });

    // ---------------------------------------------------------
    // Q12 — Unknown resources cannot receive vendors
    // ---------------------------------------------------------

    assertThrows<NotFoundError>([&] {
        (void)(acq.addVendor(
            "Q12-UNKNOWN",
            "Unknown Vendor",
            Money::of(100)));
    });

    assertThrows<NotFoundError>([&] {
        (void)(acq.cheapestVendor("Q12-UNKNOWN"));
    });
}

void testCatalogSearches() {
    Catalog catalog;

    catalog.emplace<Book>(
        "B101",
        "Clean Code",
        std::vector<std::string>{"Robert C. Martin"},
        "978-0132350884",
        "Prentice Hall",
        2008,
        Money::of(500),
        1,
        Binding::Paperback);

    catalog.emplace<Book>(
        "B102",
        "The C++ Programming Language",
        std::vector<std::string>{"Bjarne Stroustrup"},
        "978-0321563842",
        "Addison-Wesley",
        2013,
        Money::of(700),
        4,
        Binding::Hardcover);

    catalog.emplace<Journal>(
        "J101",
        "Journal of Computer Science",
        "1234-5678",
        12,
        "Academic Press",
        2020,
        Money::of(1000),
        1);

    // =========================================================
    // Q13-A: Search by author
    // =========================================================

    auto authorResults =
        catalog.searchAuthor("Robert C. Martin");

    assert(authorResults.size() == 1);
    assert(authorResults[0]->id() == "B101");

    // Case-insensitive search.
    auto caseInsensitiveAuthor =
        catalog.searchAuthor("robert c. martin");

    assert(caseInsensitiveAuthor.size() == 1);
    assert(caseInsensitiveAuthor[0]->id() == "B101");

    // Partial author search.
    auto partialAuthor =
        catalog.searchAuthor("stroustrup");

    assert(partialAuthor.size() == 1);
    assert(partialAuthor[0]->id() == "B102");

    // Unknown author.
    auto missingAuthor =
        catalog.searchAuthor("Unknown Author");

    assert(missingAuthor.empty());

    // =========================================================
    // Q13-B: Search by ISBN / ISSN
    // =========================================================

    auto isbnResults =
        catalog.searchIsbnIssn("978-0132350884");

    assert(isbnResults.size() == 1);
    assert(isbnResults[0]->id() == "B101");

    auto secondIsbnResults =
        catalog.searchIsbnIssn("978-0321563842");

    assert(secondIsbnResults.size() == 1);
    assert(secondIsbnResults[0]->id() == "B102");

    auto issnResults =
        catalog.searchIsbnIssn("1234-5678");

    assert(issnResults.size() == 1);
    assert(issnResults[0]->id() == "J101");

    // Unknown ISBN/ISSN.
    auto missingIdentifier =
        catalog.searchIsbnIssn("0000-0000");

    assert(missingIdentifier.empty());

    // =========================================================
    // Q13-C: Search by publication year range
    // =========================================================

    auto rangeResults =
        catalog.searchYearRange(2013, 2020);

    assert(rangeResults.size() == 2);
    assert(rangeResults[0]->id() == "B102");
    assert(rangeResults[1]->id() == "J101");

    // Inclusive single-year range.
    auto singleYear =
        catalog.searchYearRange(2013, 2013);

    assert(singleYear.size() == 1);
    assert(singleYear[0]->id() == "B102");

    // Range covering all resources.
    auto allYears =
        catalog.searchYearRange(2008, 2020);

    assert(allYears.size() == 3);

    // Range with no matching resources.
    auto noResults =
        catalog.searchYearRange(2021, 2025);

    assert(noResults.empty());

    // Invalid range.
    assertThrows<std::invalid_argument>([&] {
        (void)(catalog.searchYearRange(2025, 2020));
    });
}

void testLending() {
    Catalog catalog;

    // Print resource with 2 copies
    catalog.emplace<Book>(
        "B201",
        "Clean Code",
        std::vector<std::string>{"Robert C. Martin"},
        "978-0132350884",
        "Prentice Hall",
        2008,
        Money::of(500),
        1,
        Binding::Paperback);

    catalog.addHoldings("B201", 2);

    // Electronic resource with 2 licensed seats
    catalog.emplace<EBook>(
        "E201",
        "Effective Modern C++",
        std::vector<std::string>{"Scott Meyers"},
        "978-1491903995",
        "O'Reilly",
        2014,
        Money::of(600),
        "https://example.com/ebook",
        LicenseModel::AnnualSubscription,
        Money::of(50),
        FileFormat::PDF,
        true);

    catalog.addHoldings("E201", 2);

    LendingManager lending(catalog);

    // =========================================================
    // Q14-A: Borrow and return print copies
    // =========================================================

    assert(lending.borrowedCopies("B201") == 0);

    lending.borrowCopy("Alice", "B201");
    assert(lending.borrowedCopies("B201") == 1);

    lending.borrowCopy("Bob", "B201");
    assert(lending.borrowedCopies("B201") == 2);

    // No more copies available
    assertThrows<std::invalid_argument>([&] {
        (void)(lending.borrowCopy("Charlie", "B201"));
    });

    // Same patron cannot borrow the same copy/resource again
    assertThrows<std::invalid_argument>([&] {
        (void)(lending.borrowCopy("Alice", "B201"));
    });

    // Return one copy
    lending.returnCopy("Alice", "B201");
    assert(lending.borrowedCopies("B201") == 1);

    // Now one copy is available again
    lending.borrowCopy("Charlie", "B201");
    assert(lending.borrowedCopies("B201") == 2);

    // Patron cannot return a copy they did not borrow
    assertThrows<std::invalid_argument>([&] {
        (void)(lending.returnCopy("David", "B201"));
    });

    // =========================================================
    // Q14-B: Open and close electronic sessions
    // =========================================================

    assert(lending.openSessions("E201") == 0);

    lending.openSession("Alice", "E201");
    assert(lending.openSessions("E201") == 1);

    lending.openSession("Bob", "E201");
    assert(lending.openSessions("E201") == 2);

    // No licensed seats available
    assertThrows<std::invalid_argument>([&] {
        (void)(lending.openSession("Charlie", "E201"));
    });

    // Same patron cannot open another session
    assertThrows<std::invalid_argument>([&] {
        (void)(lending.openSession("Alice", "E201"));
    });

    // Close one session
    lending.closeSession("Alice", "E201");
    assert(lending.openSessions("E201") == 1);

    // Seat becomes available
    lending.openSession("Charlie", "E201");
    assert(lending.openSessions("E201") == 2);

    // Patron cannot close a session they do not have
    assertThrows<std::invalid_argument>([&] {
        (void)(lending.closeSession("David", "E201"));
    });

    // =========================================================
    // Q14-C: Wrong operation type
    // =========================================================

    // Print resources use borrowing, not electronic sessions
    assertThrows<std::invalid_argument>([&] {
        (void)(lending.openSession("Alice", "B201"));
    });

    // Electronic resources use sessions, not print borrowing
    assertThrows<std::invalid_argument>([&] {
        (void)(lending.borrowCopy("Alice", "E201"));
    });

    // =========================================================
    // Q14-D: Invalid resource
    // =========================================================

    assertThrows<NotFoundError>([&] {
        (void)(lending.borrowCopy("Alice", "INVALID"));
    });

    assertThrows<NotFoundError>([&] {
        (void)(lending.openSession("Alice", "INVALID"));
    });

    // =========================================================
    // Q14-E: Empty patron
    // =========================================================

    assertThrows<std::invalid_argument>([&] {
        (void)(lending.borrowCopy("", "B201"));
    });

    assertThrows<std::invalid_argument>([&] {
        (void)(lending.openSession("", "E201"));
    });
}

int main() {
    testMoney();
    testResourcesAndCost();
    testCatalog();
    testBudget();
    testAcquisition();
    testTaxes();
    testPostTaxQuota();
    testTitleLimits();
    testAcquisitionTitleLimits();
    testCancellation();
    testDepartmentBudgets();
    testBudgetRollover();
    testAllOrNothingBatch();
    testVendors();
    testCatalogSearches();
    testLending();
    testMoneyCurrencies();

    std::cout << "All tests passed\n";

    return 0;
}