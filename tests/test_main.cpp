// Minimal self-contained test runner (no external framework needed).
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
                      << "\n";                                                   \
        }                                                                        \
    } while (0)

#define CHECK_THROWS(expr, ExType)            \
    do {                                      \
        bool thrown_ = false;                 \
        try {                                 \
            (void)(expr);                     \
        } catch (const ExType&) {             \
            thrown_ = true;                   \
        } catch (...) {                       \
        }                                     \
        CHECK(thrown_ && "expected " #ExType); \
    } while (0)

static void testMoney() {
    CHECK(Money::of(12, 5).toString() == "12.05");
    CHECK(Money::of(-3, 50).toString() == "-3.50");
    CHECK(Money::fromMinor(7).toString() == "0.07");
    CHECK(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    CHECK(Money::of(3) * 4 == Money::of(12));
    CHECK(Money::of(1) < Money::of(2));
    CHECK_THROWS(Money::of(1, 100), std::invalid_argument);
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));

    CHECK(b.category() == ResourceCategory::Book);
    CHECK(!b.isDigital());
    CHECK(b.costFor(3) == Money::of(300));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");

    Journal j("J1", "ACM Computing Surveys", "0360-0300", 4,
              "ACM", 2026, Money::of(500), 3);

    CHECK(j.category() == ResourceCategory::Journal);
    CHECK(!j.isDigital());
    CHECK(j.issn() == "0360-0300");
    CHECK(j.issuesPerYear() == 4);
    CHECK(j.subscriptionYears() == 3);

    // 500 per copy per year × 2 copies × 3 years.
    CHECK(j.costFor(2) == Money::of(3000));

    // Default subscription length is one year.
    Journal oneYear("J2", "Nature", "0028-0836", 52,
                    "Springer", 2026, Money::of(800));

    CHECK(oneYear.subscriptionYears() == 1);
    CHECK(oneYear.costFor(2) == Money::of(1600));

    CHECK_THROWS(
        Journal("J3", "Invalid Journal", "0000-0000", 12,
                "Publisher", 2026, Money::of(100), 0),
        std::invalid_argument);

    CHECK_THROWS(
        j.costFor(0),
        std::invalid_argument);

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

    CHECK(ebook.category() == ResourceCategory::EBook);
    CHECK(ebook.isDigital());
    CHECK(ebook.isbn() == "978-1234567890");
    CHECK(ebook.format() == FileFormat::EPUB);
    CHECK(ebook.drmProtected());
    CHECK(ebook.authors().size() == 2);

    // Pricing must remain inherited from ElectronicResource.
    CHECK(ebook.costFor(4) == Money::of(1700));

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
        Binding::Paperback
    );

    Book hardcover(
        "B-Q4-H",
        "Q4 Hardcover",
        {"Author"},
        "ISBN-Q4-H",
        "Publisher",
        2026,
        Money::of(450),
        1,
        Binding::Hardcover
    );

    CHECK(paperback.costFor(1) == Money::of(450));
    CHECK(hardcover.costFor(1) == Money::of(540));
    CHECK(paperback.costFor(2) == Money::of(900));
    CHECK(hardcover.costFor(2) == Money::of(1080));

    // Q5 — Bulk discounts for print items
    Book q5Paperback(
        "B-Q5-P",
        "Q5 Paperback",
        {"Author"},
        "ISBN-Q5-P",
        "Publisher",
        2026,
        Money::of(100),
        1,
        Binding::Paperback
    );

    Book q5Hardcover(
        "B-Q5-H",
        "Q5 Hardcover",
        {"Author"},
        "ISBN-Q5-H",
        "Publisher",
        2026,
        Money::of(100),
        1,
        Binding::Hardcover
    );

    CHECK(q5Paperback.costFor(9) == Money::of(900));
    CHECK(q5Paperback.costFor(10) == Money::of(1000));
    CHECK(q5Hardcover.costFor(9) == Money::of(1080));
    CHECK(q5Hardcover.costFor(10) == Money::of(1200));

    // Q5 — Bulk discount for Journal
    Journal q5Journal(
        "J-Q5",
        "Q5 Journal",
        "Publisher",
        2026,
        "ISSN-Q5",
        12,
        Money::of(100),
        1
    );

    CHECK(q5Journal.costFor(9) == Money::of(900));
    CHECK(q5Journal.costFor(10) == Money::of(1000));

    // Q5 — Electronic resource: 50-seat boundary
    ElectronicResource q5Electronic(
        "ER-Q5",
        "Q5 Electronic Resource",
        "Publisher",
        2026,
        Money::of(100),
        "https://example.com/q5",
        LicenseModel::AnnualSubscription,
        Money::of(500)
    );

    CHECK(q5Electronic.costFor(50) == Money::of(5500));
    CHECK(q5Electronic.costFor(51) == Money::of(5550));
    CHECK(q5Electronic.costFor(60) == Money::of(6000));

    ElectronicResource e(
        "R1",
        "DB",
        "P",
        2026,
        Money::of(10),
        "url",
        LicenseModel::AnnualSubscription,
        Money::of(100));

    CHECK(e.isDigital());
    CHECK(e.costFor(5) == Money::of(150));
    CHECK(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& journalResource = j;

    CHECK(journalResource.category() == ResourceCategory::Journal);
    CHECK(journalResource.costFor(2) == Money::of(3000));

    const ElectronicResource& electronic = ebook;

    CHECK(electronic.costFor(4) == Money::of(1700));
    CHECK(electronic.category() == ResourceCategory::EBook);

    const Resource& r = e;

    CHECK(r.costFor(1) == Money::of(110));

    std::ostringstream os;
    os << r;

    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);

    CHECK_THROWS(
        Book("", "T", {}, "", "", 2000, Money::of(1)),
        std::invalid_argument);

    CHECK_THROWS(
        Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)),
        std::invalid_argument);
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

    CHECK(c.size() == 3);
    CHECK(c.contains("B1"));
    CHECK(c.find("nope") == nullptr);
    CHECK_THROWS(c.get("nope"), NotFoundError);

    CHECK_THROWS(
        c.emplace<Book>(
            "B1",
            "dup",
            std::vector<std::string>{},
            "",
            "",
            1,
            Money::of(1)),
        DuplicateIdError);

    CHECK(c.searchTitle("clean").size() == 2);
    CHECK(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    CHECK(c.where([](const Resource& r) {
        return r.isDigital();
    }).size() == 1);

    CHECK(c.holdings("B1") == 0);

    c.addHoldings("B1", 3);

    CHECK(c.holdings("B1") == 3);
    CHECK_THROWS(c.addHoldings("B1", -5), std::invalid_argument);

    c.remove("R1");

    CHECK(c.size() == 2);
    CHECK_THROWS(c.remove("R1"), NotFoundError);
}

static void testBudget() {
    Budget b(Money::of(1000));

    b.setQuota(
        ResourceCategory::Book,
        {5, Money::of(400)});

    b.setQuota(
        ResourceCategory::Journal,
        {4, Money::of(3000)});

    CHECK(
        b.check(
            ResourceCategory::Journal,
            3,
            Money::of(800)).empty());

    CHECK(
        !b.check(
            ResourceCategory::Journal,
            5,
            Money::of(100)).empty());

    CHECK(
        !b.check(
            ResourceCategory::Journal,
            1,
            Money::of(3001)).empty());

    CHECK(
        b.check(
            ResourceCategory::Book,
            2,
            Money::of(200)).empty());

    CHECK(
        !b.check(
            ResourceCategory::Book,
            6,
            Money::of(10)).empty());

    CHECK(
        !b.check(
            ResourceCategory::Book,
            1,
            Money::of(401)).empty());

    CHECK(
        !b.check(
            ResourceCategory::ElectronicResource,
            1,
            Money::of(1001)).empty());

    CHECK(
        b.check(
            ResourceCategory::ElectronicResource,
            1,
            Money::of(900)).empty());

    b.commit(
        ResourceCategory::Book,
        4,
        Money::of(300));

    CHECK(b.spent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    CHECK_THROWS(
        b.commit(
            ResourceCategory::Book,
            2,
            Money::of(10)),
        QuotaExceededError);

    CHECK_THROWS(
        b.commit(
            ResourceCategory::ElectronicResource,
            1,
            Money::of(800)),
        BudgetExceededError);

    CHECK_THROWS(
        b.commit(
            ResourceCategory::ElectronicResource,
            0,
            Money::of(1)),
        std::invalid_argument);

    CHECK(b.spent() == Money::of(300));

    Budget ebookBudget(Money::of(25000));

    ebookBudget.setQuota(
        ResourceCategory::EBook,
        {5, Money::of(2500)});

    CHECK(
        ebookBudget.check(
            ResourceCategory::EBook,
            3,
            Money::of(1700)).empty());

    CHECK(
        !ebookBudget.check(
            ResourceCategory::EBook,
            6,
            Money::of(1700)).empty());

    CHECK(
        !ebookBudget.check(
            ResourceCategory::EBook,
            2,
            Money::of(2600)).empty());

    // Q3: AudioBook
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

    CHECK(audiobook.category() == ResourceCategory::AudioBook);
    CHECK(audiobook.isDigital());
    CHECK(audiobook.narrator() == "Jane Smith");
    CHECK(audiobook.durationMinutes() == 360);
    CHECK(audiobook.costFor(4) == Money::of(1100));

    const ElectronicResource& audioElectronic = audiobook;

    CHECK(audioElectronic.costFor(4) == Money::of(1100));

    CHECK_THROWS(
        AudioBook(
            "A2",
            "Invalid Audio",
            "Jane Smith",
            0,
            "Audio Press",
            2026,
            Money::of(200),
            "https://audio.example/invalid"),
        std::invalid_argument);

    // Q3: Thesis
    Thesis thesis(
        "T1",
        "Efficient Algorithms",
        "ABC University",
        "M.Tech",
        "Dr. Rao",
        2026);

    CHECK(thesis.category() == ResourceCategory::Thesis);
    CHECK(!thesis.isDigital());
    CHECK(thesis.university() == "ABC University");
    CHECK(thesis.degree() == "M.Tech");
    CHECK(thesis.supervisor() == "Dr. Rao");
    CHECK(thesis.costFor(1) == Money{});
    CHECK(thesis.costFor(5) == Money{});

    Resource& thesisResource = thesis;

    CHECK(thesisResource.category() == ResourceCategory::Thesis);
    CHECK(thesisResource.costFor(2) == Money{});
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

    CHECK(acq.quote("R1", 5) == Money::of(100));

    std::string why;

    CHECK(acq.canPurchase("B1", 3, &why) && why.empty());
    CHECK(!acq.canPurchase("B1", 4, &why) && !why.empty());
    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);

    CHECK(
        rec.approved &&
        rec.cost == Money::of(200) &&
        rec.orderNo == 1);

    CHECK(c.holdings("B1") == 2);

    CHECK_THROWS(
        acq.purchase("B1", 2),
        QuotaExceededError);

    CHECK_THROWS(
        acq.purchase("nope", 1),
        NotFoundError);

    CHECK(acq.history().size() == 1);

    auto res = acq.processBatch({
        {"R1", 10},
        {"R1", 100},
        {"B1", 1},
        {"zzz", 1},
        {"B1", 0}
    });

    CHECK(res.size() == 5);
    CHECK(res[0].approved && res[0].cost == Money::of(150));
    CHECK(!res[1].approved);
    CHECK(res[2].approved);
    CHECK(
        !res[3].approved &&
        res[3].reason.find("not found") != std::string::npos);
    CHECK(!res[4].approved);

    CHECK(acq.totalSpent() == Money::of(450));
    CHECK(b.spent() == acq.totalSpent());
    CHECK(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    CHECK(acq.history().size() == 6);
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

    CHECK(acq.printTaxPercent() == 10);
    CHECK(acq.electronicTaxPercent() == 20);

    CHECK(acq.quote("Q6-B", 1) == Money::of(110));
    CHECK(acq.quote("Q6-E", 1) == Money::of(120));

    const auto& bookRecord = acq.purchase("Q6-B", 1);

    CHECK(bookRecord.preTaxCost == Money::of(100));
    CHECK(bookRecord.tax == Money::of(10));
    CHECK(bookRecord.cost == Money::of(110));

    CHECK(b.spent() == Money::of(110));
    CHECK(
        b.usageFor(ResourceCategory::Book).spent ==
        Money::of(110));

    acq.setTaxRates(5, 15);

    CHECK(acq.printTaxPercent() == 5);
    CHECK(acq.electronicTaxPercent() == 15);

    CHECK(acq.quote("Q6-B", 1) == Money::of(105));
    CHECK(acq.quote("Q6-E", 1) == Money::of(115));

    CHECK_THROWS(
        acq.setTaxRates(-1, 10),
        std::invalid_argument);

    CHECK_THROWS(
        acq.setTaxRates(10, 101),
        std::invalid_argument);
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

    CHECK(acq.quote("Q6-Q", 1) == Money::of(110));

    std::string reason;

    CHECK(!acq.canPurchase("Q6-Q", 1, &reason));
    CHECK(!reason.empty());

    CHECK(b.spent() == Money{});
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

    CHECK(
        b.titleLimitFor(
            ResourceCategory::Book).has_value());

    CHECK(
        *b.titleLimitFor(
            ResourceCategory::Book) == 2);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 0);

    CHECK(
        b.checkTitle(
            ResourceCategory::Book,
            "Q7-B1").empty());

    b.commitTitle(
        ResourceCategory::Book,
        "Q7-B1");

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    CHECK(
        b.checkTitle(
            ResourceCategory::Book,
            "Q7-B1").empty());

    b.commitTitle(
        ResourceCategory::Book,
        "Q7-B1");

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    CHECK(
        b.checkTitle(
            ResourceCategory::Book,
            "Q7-B2").empty());

    b.commitTitle(
        ResourceCategory::Book,
        "Q7-B2");

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(
        !b.checkTitle(
            ResourceCategory::Book,
            "Q7-B3").empty());

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(
        !b.titleLimitFor(
            ResourceCategory::Journal).has_value());

    CHECK(
        b.checkTitle(
            ResourceCategory::Journal,
            "Q7-J1").empty());

    Budget zeroTitleBudget(Money::of(1000));

    zeroTitleBudget.setTitleLimit(
        ResourceCategory::Book,
        0);

    CHECK(
        !zeroTitleBudget.checkTitle(
            ResourceCategory::Book,
            "Q7-B1").empty());

    CHECK(
        zeroTitleBudget.titlesUsed(
            ResourceCategory::Book) == 0);

    CHECK_THROWS(
        b.setTitleLimit(
            ResourceCategory::Book,
            -1),
        std::invalid_argument);
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

    CHECK(
        acq.canPurchase(
            "Q7-B1",
            1,
            &reason));

    CHECK(reason.empty());

    const auto& first =
        acq.purchase("Q7-B1", 1);

    CHECK(first.approved);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    CHECK(
        acq.canPurchase(
            "Q7-B1",
            2,
            &reason));

    CHECK(reason.empty());

    const auto& sameTitle =
        acq.purchase("Q7-B1", 2);

    CHECK(sameTitle.approved);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    CHECK(
        acq.canPurchase(
            "Q7-B2",
            1,
            &reason));

    CHECK(reason.empty());

    const auto& second =
        acq.purchase("Q7-B2", 1);

    CHECK(second.approved);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(
        !acq.canPurchase(
            "Q7-B3",
            1,
            &reason));

    CHECK(!reason.empty());

    CHECK(
        reason.find("title") !=
        std::string::npos);

    CHECK_THROWS(
        acq.purchase("Q7-B3", 1),
        QuotaExceededError);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(
        c.holdings("Q7-B3") == 0);

    auto results = acq.processBatch({
        {"Q7-B3", 1},
        {"Q7-B1", 1}
    });

    CHECK(results.size() == 2);
    CHECK(!results[0].approved);

    CHECK(
        results[0].reason.find("title") !=
        std::string::npos);

    CHECK(results[1].approved);

    CHECK(
        c.holdings("Q7-B1") == 4);

    CHECK(
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

    CHECK(first.approved && !first.cancellation);
    CHECK(second.approved && !second.cancellation);
    CHECK(other.approved && !other.cancellation);

    CHECK(b.spent() == Money::of(440));

    CHECK(
        b.usageFor(
            ResourceCategory::Book).units == 4);

    CHECK(
        b.usageFor(
            ResourceCategory::Book).spent ==
        Money::of(440));

    CHECK(c.holdings("Q8-B1") == 3);
    CHECK(c.holdings("Q8-B2") == 1);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(acq.totalSpent() == Money::of(440));

    // Cancelling one of two purchases of B1 must not release its title slot.
    PurchaseRecord cancellation1 =
        acq.cancel(first.orderNo);

    CHECK(cancellation1.cancellation);
    CHECK(cancellation1.approved);
    CHECK(cancellation1.resourceId == "Q8-B1");
    CHECK(cancellation1.quantity == 2);
    CHECK(cancellation1.cost == Money{});

    CHECK(
        cancellation1.reason.find("cancelled order") !=
        std::string::npos);

    CHECK(b.spent() == Money::of(220));

    CHECK(
        b.usageFor(
            ResourceCategory::Book).units == 2);

    CHECK(
        b.usageFor(
            ResourceCategory::Book).spent ==
        Money::of(220));

    CHECK(c.holdings("Q8-B1") == 1);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(acq.totalSpent() == Money::of(220));
    CHECK(acq.history().size() == 4);

    // The original purchase remains in history.
    CHECK(
        acq.history()[0].orderNo ==
        first.orderNo);

    CHECK(acq.history()[0].approved);
    CHECK(!acq.history()[0].cancellation);

    // Cancelling the last active B1 purchase releases its title slot.
    PurchaseRecord cancellation2 =
        acq.cancel(second.orderNo);

    CHECK(cancellation2.cancellation);

    CHECK(b.spent() == Money::of(110));

    CHECK(
        b.usageFor(
            ResourceCategory::Book).units == 1);

    CHECK(
        b.usageFor(
            ResourceCategory::Book).spent ==
        Money::of(110));

    CHECK(c.holdings("Q8-B1") == 0);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    CHECK(acq.totalSpent() == Money::of(110));

    // B3 can now use the title slot released by B1.
    CHECK(acq.canPurchase("Q8-B3", 1));

    const auto& third =
        acq.purchase("Q8-B3", 1);

    CHECK(third.approved);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(c.holdings("Q8-B3") == 1);

    // A third different active title is rejected by Q7.
    auto rejected =
        acq.processBatch({{"Q8-B4", 1}});

    CHECK(rejected.size() == 1);
    CHECK(!rejected[0].approved);
    CHECK(!rejected[0].cancellation);

    CHECK(
        rejected[0].reason.find("title") !=
        std::string::npos);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(c.holdings("Q8-B4") == 0);

    // A rejected order cannot be cancelled.
    CHECK_THROWS(
        acq.cancel(rejected[0].orderNo),
        std::invalid_argument);

    // An already cancelled order cannot be cancelled again.
    CHECK_THROWS(
        acq.cancel(first.orderNo),
        std::invalid_argument);

    // A cancellation record cannot itself be cancelled.
    CHECK_THROWS(
        acq.cancel(cancellation1.orderNo),
        std::invalid_argument);

    // Unknown order numbers are rejected.
    CHECK_THROWS(
        acq.cancel(999999),
        NotFoundError);

    // Cancel the remaining active titles and verify all Book usage is refunded.
    acq.cancel(other.orderNo);
    acq.cancel(third.orderNo);

    CHECK(b.spent() == Money{});

    CHECK(
        b.usageFor(
            ResourceCategory::Book).units == 0);

    CHECK(
        b.usageFor(
            ResourceCategory::Book).spent ==
        Money{});

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 0);

    CHECK(c.holdings("Q8-B1") == 0);
    CHECK(c.holdings("Q8-B2") == 0);
    CHECK(c.holdings("Q8-B3") == 0);

    CHECK(acq.totalSpent() == Money{});
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

    std::cout << (g_checks - g_failures)
              << "/" << g_checks
              << " checks passed\n";

    return g_failures == 0 ? 0 : 1;
}