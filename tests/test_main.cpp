// Minimal self-contained test runner (no external framework needed).

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        ++g_checks;                                                        \
        if (!(cond)) {                                                     \
            ++g_failures;                                                  \
            std::cerr << __FILE__ << ":" << __LINE__                       \
                      << ": CHECK failed: " #cond << "\n";                 \
        }                                                                  \
    } while (0)

#define CHECK_THROWS(expr, ExType)             \
    do {                                       \
        bool thrown_ = false;                  \
        try {                                  \
            (void)(expr);                      \
        } catch (const ExType&) {              \
            thrown_ = true;                    \
        } catch (...) {                        \
        }                                      \
        CHECK(thrown_ && "expected " #ExType); \
    } while (0)

static void testMoney() {
    CHECK(Money::of(12, 5).toString() == "12.05");
    CHECK(Money::of(-3, 50).toString() == "-3.50");
    CHECK(Money::fromMinor(7).toString() == "0.07");
    CHECK(Money::of(10) + Money::of(0, 50) ==
          Money::fromMinor(1050));
    CHECK(Money::of(3) * 4 == Money::of(12));
    CHECK(Money::of(1) < Money::of(2));
    CHECK_THROWS(Money::of(1, 100), std::invalid_argument);
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

    CHECK(b.category() == ResourceCategory::Book);
    CHECK(!b.isDigital());
    CHECK(b.costFor(3) == Money::of(300));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");

    Journal j(
        "J1",
        "ACM Computing Surveys",
        "0360-0300",
        4,
        "ACM",
        2026,
        Money::of(500),
        3);

    CHECK(j.category() == ResourceCategory::Journal);
    CHECK(!j.isDigital());
    CHECK(j.issn() == "0360-0300");
    CHECK(j.issuesPerYear() == 4);
    CHECK(j.subscriptionYears() == 3);

    // 500 per copy per year × 2 copies × 3 years.
    CHECK(j.costFor(2) == Money::of(3000));

    // Default subscription length is one year.
    Journal oneYear(
        "J2",
        "Nature",
        "0028-0836",
        52,
        "Springer",
        2026,
        Money::of(800));

    CHECK(oneYear.subscriptionYears() == 1);
    CHECK(oneYear.costFor(2) == Money::of(1600));

    CHECK_THROWS(
        Journal(
            "J3",
            "Invalid Journal",
            "0000-0000",
            12,
            "Publisher",
            2026,
            Money::of(100),
            0),
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

    CHECK(paperback.costFor(1) == Money::of(450));
    CHECK(hardcover.costFor(1) == Money::of(540));
    CHECK(paperback.costFor(2) == Money::of(900));
    CHECK(hardcover.costFor(2) == Money::of(1080));

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

    CHECK(q5Paperback.costFor(9) == Money::of(900));
    CHECK(q5Paperback.costFor(10) == Money::of(1000));
    CHECK(q5Hardcover.costFor(9) == Money::of(1080));
    CHECK(q5Hardcover.costFor(10) == Money::of(1200));

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

    CHECK(q5Journal.costFor(9) == Money::of(900));
    CHECK(q5Journal.costFor(10) == Money::of(1000));

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

    // Polymorphism through a base-class reference.
    const Resource& journalResource = j;

    CHECK(journalResource.category() ==
          ResourceCategory::Journal);
    CHECK(journalResource.costFor(2) == Money::of(3000));

    const ElectronicResource& electronic = ebook;

    CHECK(electronic.costFor(4) == Money::of(1700));
    CHECK(electronic.category() == ResourceCategory::EBook);

    const Resource& r = e;

    CHECK(r.costFor(1) == Money::of(110));

    std::ostringstream os;
    os << r;

    CHECK(os.str().find("platform fee: 100.00") !=
          std::string::npos);

    CHECK_THROWS(
        Book("", "T", {}, "", "", 2000, Money::of(1)),
        std::invalid_argument);

    CHECK_THROWS(
        Book(
            "B",
            "T",
            {},
            "",
            "",
            2000,
            Money::fromMinor(-1)),
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
    CHECK(c.byCategory(ResourceCategory::ElectronicResource)
              .size() == 1);

    CHECK(c.where([](const Resource& r) {
        return r.isDigital();
    }).size() == 1);

    CHECK(c.holdings("B1") == 0);

    c.addHoldings("B1", 3);

    CHECK(c.holdings("B1") == 3);
    CHECK_THROWS(
        c.addHoldings("B1", -5),
        std::invalid_argument);

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
             Money::of(800))
            .empty());

    CHECK(
        !b.check(
             ResourceCategory::Journal,
             5,
             Money::of(100))
             .empty());

    CHECK(
        !b.check(
             ResourceCategory::Journal,
             1,
             Money::of(3001))
             .empty());

    CHECK(
        b.check(
             ResourceCategory::Book,
             2,
             Money::of(200))
            .empty());

    CHECK(
        !b.check(
             ResourceCategory::Book,
             6,
             Money::of(10))
             .empty());

    CHECK(
        !b.check(
             ResourceCategory::Book,
             1,
             Money::of(401))
             .empty());

    CHECK(
        !b.check(
             ResourceCategory::ElectronicResource,
             1,
             Money::of(1001))
             .empty());

    CHECK(
        b.check(
             ResourceCategory::ElectronicResource,
             1,
             Money::of(900))
            .empty());

    b.commit(
        ResourceCategory::Book,
        4,
        Money::of(300));

    CHECK(b.spent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) ==
          Money::of(100));
    CHECK(
        !b.unitsRemaining(
             ResourceCategory::ElectronicResource)
             .has_value());

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
                       Money::of(1700))
            .empty());

    CHECK(
        !ebookBudget.check(
             ResourceCategory::EBook,
             6,
             Money::of(1700))
             .empty());

    CHECK(
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

    CHECK(audiobook.category() ==
          ResourceCategory::AudioBook);
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

    // Q3: Thesis.
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

    CHECK(thesisResource.category() ==
          ResourceCategory::Thesis);
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

    CHECK(
        acq.canPurchase("B1", 3, &why) &&
        why.empty());

    CHECK(
        !acq.canPurchase("B1", 4, &why) &&
        !why.empty());

    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto& rec =
        acq.purchase("B1", 2);

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
        {"Default", "R1", 10},
        {"Default", "R1", 100},
        {"Default", "B1", 1},
        {"Default", "zzz", 1},
        {"Default", "B1", 0}
    });

    CHECK(res.size() == 5);
    CHECK(
        res[0].approved &&
        res[0].cost == Money::of(150));
    CHECK(!res[1].approved);
    CHECK(res[2].approved);

    CHECK(
        !res[3].approved &&
        res[3].reason.find("not found") !=
            std::string::npos);

    CHECK(!res[4].approved);

    CHECK(acq.totalSpent() == Money::of(450));
    CHECK(b.spent() == acq.totalSpent());
    CHECK(
        c.holdings("R1") == 10 &&
        c.holdings("B1") == 3);
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

    const auto& bookRecord =
        acq.purchase("Q6-B", 1);

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

    CHECK(
        !acq.canPurchase(
            "Q6-Q",
            1,
            &reason));

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
            ResourceCategory::Book)
            .has_value());

    CHECK(
        *b.titleLimitFor(
            ResourceCategory::Book) == 2);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 0);

    CHECK(
        b.checkTitle(
            ResourceCategory::Book,
            "Q7-B1")
            .empty());

    b.commitTitle(
        ResourceCategory::Book,
        "Q7-B1");

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    CHECK(
        b.checkTitle(
            ResourceCategory::Book,
            "Q7-B1")
            .empty());

    b.commitTitle(
        ResourceCategory::Book,
        "Q7-B1");

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 1);

    CHECK(
        b.checkTitle(
            ResourceCategory::Book,
            "Q7-B2")
            .empty());

    b.commitTitle(
        ResourceCategory::Book,
        "Q7-B2");

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(
        !b.checkTitle(
             ResourceCategory::Book,
             "Q7-B3")
             .empty());

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(
        !b.titleLimitFor(
             ResourceCategory::Journal)
             .has_value());

    CHECK(
        b.checkTitle(
            ResourceCategory::Journal,
            "Q7-J1")
            .empty());

    Budget zeroTitleBudget(Money::of(1000));

    zeroTitleBudget.setTitleLimit(
        ResourceCategory::Book,
        0);

    CHECK(
        !zeroTitleBudget.checkTitle(
             ResourceCategory::Book,
             "Q7-B1")
             .empty());

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

    CHECK(c.holdings("Q7-B3") == 0);

    auto results = acq.processBatch({
        {"Default", "Q7-B3", 1},
        {"Default", "Q7-B1", 1}
    });

    CHECK(results.size() == 2);
    CHECK(!results[0].approved);

    CHECK(
        results[0].reason.find("title") !=
        std::string::npos);

    CHECK(results[1].approved);

    CHECK(c.holdings("Q7-B1") == 4);

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

    // Cancelling one of two purchases of B1 must not release
    // its title slot.
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
        acq.processBatch({
            {"Default", "Q8-B4", 1}
        });

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

    // Cancel the remaining active titles and verify
    // all Book usage is refunded.
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

    CHECK(
        &acq.departmentBudget("Computer Science") ==
        &csBudget);

    CHECK(
        &acq.departmentBudget("Physics") ==
        &physicsBudget);

    CHECK(
        &acq.departmentBudget("Default") ==
        &defaultBudget);

    CHECK_THROWS(
        acq.departmentBudget("Mathematics"),
        NotFoundError);

    // ---------------------------------------------------------
    // Same resource, different departments
    // ---------------------------------------------------------

    CHECK(
        acq.quote(
            "Computer Science",
            "Q9-B1",
            2) ==
        Money::of(200));

    CHECK(
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

    CHECK(csFirst.approved);
    CHECK(csFirst.department == "Computer Science");
    CHECK(csBudget.spent() == Money::of(200));

    CHECK(
        csBudget.usageFor(
                    ResourceCategory::Book)
            .units == 2);

    CHECK(c.holdings("Q9-B1") == 2);

    // The same title has an independent title slot in Physics.
    PurchaseRecord physicsFirst =
        acq.purchase(
            "Physics",
            "Q9-B1",
            1);

    CHECK(physicsFirst.approved);
    CHECK(physicsFirst.department == "Physics");
    CHECK(physicsBudget.spent() == Money::of(100));

    CHECK(
        physicsBudget.usageFor(
                        ResourceCategory::Book)
            .units == 1);

    CHECK(
        physicsBudget.titlesUsed(
            ResourceCategory::Book) == 1);

    CHECK(
        csBudget.titlesUsed(
            ResourceCategory::Book) == 1);

    // ---------------------------------------------------------
    // Physics has reached its unit quota.
    // Computer Science still has available quota.
    // ---------------------------------------------------------

    std::string reason;

    CHECK(
        !acq.canPurchase(
             "Physics",
             "Q9-B2",
             1,
             &reason));

    CHECK(!reason.empty());

    CHECK(
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

    CHECK(csSecond.approved);
    CHECK(csBudget.spent() == Money::of(400));
    CHECK(physicsBudget.spent() == Money::of(100));

    // Physics has a one-title limit, so a new title is rejected.
    CHECK(
        !acq.canPurchase(
             "Physics",
             "Q9-B2",
             1,
             &reason));

    CHECK(
        reason.find("title") !=
        std::string::npos);

    // Computer Science has a separate title limit of three.
    CHECK(
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

    CHECK(batch.size() == 3);

    // CS accepts its third different title.
    CHECK(batch[0].approved);
    CHECK(
        batch[0].department ==
        "Computer Science");

    // Physics rejects because its title limit is already full.
    CHECK(!batch[1].approved);
    CHECK(
        batch[1].department ==
        "Physics");

    CHECK(
        batch[1].reason.find("title") !=
        std::string::npos);

    // Mathematics has no registered department budget.
    CHECK(!batch[2].approved);

    CHECK(
        batch[2].reason.find("department not found") !=
        std::string::npos);

    CHECK(csBudget.spent() == Money::of(700));
    CHECK(physicsBudget.spent() == Money::of(100));
    CHECK(c.holdings("Q9-B3") == 1);

    CHECK(
        csBudget.titlesUsed(
            ResourceCategory::Book) == 3);

    CHECK(
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

    CHECK(csSecondQuantity == 1);

    PurchaseRecord cancelled =
        acq.cancel(csSecondOrderNo);

    CHECK(cancelled.cancellation);
    CHECK(
        cancelled.department ==
        "Computer Science");

    // B1 (200) + B3 (300) remain active.
    // B2 (200) was cancelled.
    CHECK(csBudget.spent() == Money::of(500));

    CHECK(
        csBudget.usageFor(
                    ResourceCategory::Book)
            .units == 3);

    // B2 holdings were reduced by the cancellation.
    CHECK(c.holdings("Q9-B2") == 0);

    // Physics remains completely independent.
    CHECK(physicsBudget.spent() == Money::of(100));
    CHECK(
        physicsBudget.usageFor(
                    ResourceCategory::Book)
            .units == 1);

    // The Computer Science B2 title slot was released.
    CHECK(
        csBudget.titlesUsed(
            ResourceCategory::Book) == 2);

    // Physics still has its own B1 title slot.
    CHECK(
        physicsBudget.titlesUsed(
            ResourceCategory::Book) == 1);

    // ---------------------------------------------------------
    // Department information is retained in history.
    // ---------------------------------------------------------

    CHECK(
        acq.history()[csFirst.orderNo - 1].department ==
        "Computer Science");

    CHECK(
        acq.history().back().cancellation);

    CHECK(
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

    CHECK(current.total() == Money::of(1000));
    CHECK(current.spent() == Money::of(300));
    CHECK(current.remaining() == Money::of(700));

    CHECK(
        current.usageFor(ResourceCategory::Book).units == 3);

    CHECK(
        current.usageFor(ResourceCategory::Book).spent ==
        Money::of(300));

    CHECK(
        current.titlesUsed(ResourceCategory::Book) == 1);

    // 50% of the unspent 700 = 350.
    Budget nextYear = current.rollover(50);

    CHECK(nextYear.total() == Money::of(350));
    CHECK(nextYear.spent() == Money{});
    CHECK(nextYear.remaining() == Money::of(350));

    // Category quota configuration is carried forward.
    CHECK(
        nextYear.quotaFor(ResourceCategory::Book).has_value());

    CHECK(
        nextYear.quotaFor(ResourceCategory::Book)->maxUnits == 10);

    CHECK(
        nextYear.quotaFor(ResourceCategory::Book)->maxSpend ==
        Money::of(800));

    // Q7 title-limit configuration is carried forward.
    CHECK(
        nextYear.titleLimitFor(ResourceCategory::Book).has_value());

    CHECK(
        *nextYear.titleLimitFor(ResourceCategory::Book) == 2);

    // Actual usage starts from zero in the new year.
    CHECK(
        nextYear.usageFor(ResourceCategory::Book).units == 0);

    CHECK(
        nextYear.usageFor(ResourceCategory::Book).spent ==
        Money{});

    // Purchased-title history does not carry into the new year.
    CHECK(
        nextYear.titlesUsed(ResourceCategory::Book) == 0);

    // The original budget is unchanged.
    CHECK(current.total() == Money::of(1000));
    CHECK(current.spent() == Money::of(300));
    CHECK(current.remaining() == Money::of(700));

    CHECK(
        current.usageFor(ResourceCategory::Book).units == 3);

    CHECK(
        current.titlesUsed(ResourceCategory::Book) == 1);

    // 0% rollover creates a zero-value new budget.
    Budget zeroRollover = current.rollover(0);

    CHECK(zeroRollover.total() == Money{});
    CHECK(zeroRollover.spent() == Money{});
    CHECK(zeroRollover.remaining() == Money{});

    // 100% rollover carries the complete unspent amount.
    Budget fullRollover = current.rollover(100);

    CHECK(fullRollover.total() == Money::of(700));
    CHECK(fullRollover.spent() == Money{});
    CHECK(fullRollover.remaining() == Money::of(700));

    // Invalid rollover percentages are rejected.
    CHECK_THROWS(
        current.rollover(-1),
        std::invalid_argument);

    CHECK_THROWS(
        current.rollover(101),
        std::invalid_argument);
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

    CHECK(successfulBatch.size() == 2);

    CHECK(successfulBatch[0].approved);
    CHECK(successfulBatch[1].approved);

    CHECK(
        b.spent() == Money::of(300));

    CHECK(
        b.usageFor(
            ResourceCategory::Book).units == 2);

    CHECK(
        b.usageFor(
            ResourceCategory::Book).spent ==
        Money::of(300));

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 2);

    CHECK(c.holdings("Q11-B1") == 1);
    CHECK(c.holdings("Q11-B2") == 1);

    CHECK(acq.history().size() == 2);

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
    CHECK(failedBatch.empty());

    // Budget must be completely unchanged.
    CHECK(b.spent() == spentBefore);

    CHECK(
        b.usageFor(
            ResourceCategory::Book).units ==
        unitsBefore);

    CHECK(
        b.usageFor(
            ResourceCategory::Book).spent ==
        categorySpentBefore);

    // The new title must NOT consume a title slot.
    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) ==
        titlesBefore);

    // Holdings must be completely unchanged.
    CHECK(
        c.holdings("Q11-B1") ==
        b1HoldingsBefore);

    CHECK(
        c.holdings("Q11-B2") ==
        b2HoldingsBefore);

    CHECK(
        c.holdings("Q11-B3") ==
        b3HoldingsBefore);

    // No purchase records should be created.
    CHECK(
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

    CHECK(normalBatch.size() == 2);

    CHECK(normalBatch[0].approved);

    CHECK(!normalBatch[1].approved);

    CHECK(
        normalBatch[1].reason.find("not found") !=
        std::string::npos);

    // The valid request remains purchased.
    CHECK(c.holdings("Q11-B3") == 1);

    CHECK(
        b.spent() ==
        Money::of(600));

    CHECK(
        b.usageFor(
            ResourceCategory::Book).units == 3);

    CHECK(
        b.titlesUsed(
            ResourceCategory::Book) == 3);

    // Only the normal batch created two history records.
    CHECK(acq.history().size() == 4);
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

    CHECK(cheapest == "Vendor B");

    // ---------------------------------------------------------
    // Q12 — A more expensive vendor must not be selected
    // ---------------------------------------------------------

    acq.addVendor(
        "Q12-B1",
        "Vendor D",
        Money::of(600));

    cheapest = acq.cheapestVendor("Q12-B1");

    CHECK(cheapest == "Vendor B");

    // ---------------------------------------------------------
    // Q12 — Quote must use the cheapest vendor
    // ---------------------------------------------------------

    CHECK(
        acq.quote("Q12-B1", 1) ==
        Money::of(420));

    CHECK(
        acq.quote("Q12-B1", 2) ==
        Money::of(840));

    // ---------------------------------------------------------
    // Q12 — Purchase must use the cheapest vendor
    // ---------------------------------------------------------

    PurchaseRecord first =
        acq.purchase("Q12-B1", 2);

    CHECK(first.approved);

    CHECK(first.vendor == "Vendor B");

    CHECK(first.preTaxCost == Money::of(840));

    CHECK(first.cost == Money::of(840));

    CHECK(b.spent() == Money::of(840));

    CHECK(c.holdings("Q12-B1") == 2);

    // ---------------------------------------------------------
    // Q12 — Adding a cheaper vendor changes future purchases
    // ---------------------------------------------------------

    acq.addVendor(
        "Q12-B1",
        "Vendor E",
        Money::of(390));

    cheapest = acq.cheapestVendor("Q12-B1");

    CHECK(cheapest == "Vendor E");

    CHECK(
        acq.quote("Q12-B1", 1) ==
        Money::of(390));

    PurchaseRecord second =
        acq.purchase("Q12-B1", 1);

    CHECK(second.approved);

    CHECK(second.vendor == "Vendor E");

    CHECK(second.preTaxCost == Money::of(390));

    CHECK(second.cost == Money::of(390));

    CHECK(b.spent() == Money::of(1230));

    CHECK(c.holdings("Q12-B1") == 3);

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

    CHECK(cheapest == "Academic Supplier");

    CHECK(
        acq.quote("Q12-B2", 2) ==
        Money::of(1500));

    PurchaseRecord third =
        acq.purchase("Q12-B2", 2);

    CHECK(third.approved);

    CHECK(third.vendor == "Academic Supplier");

    CHECK(third.preTaxCost == Money::of(1500));

    CHECK(third.cost == Money::of(1500));

    CHECK(c.holdings("Q12-B2") == 2);

    CHECK(b.spent() == Money::of(2730));

    // ---------------------------------------------------------
    // Q12 — Batch purchases must also use cheapest vendors
    // ---------------------------------------------------------

    auto batch = acq.processBatch({
        {"Default", "Q12-B1", 1},
        {"Default", "Q12-B2", 1}
    });

    CHECK(batch.size() == 2);

    CHECK(batch[0].approved);
    CHECK(batch[1].approved);

    CHECK(batch[0].vendor == "Vendor E");
    CHECK(batch[1].vendor == "Academic Supplier");

    CHECK(
        batch[0].preTaxCost ==
        Money::of(390));

    CHECK(
        batch[1].preTaxCost ==
        Money::of(750));

    CHECK(c.holdings("Q12-B1") == 4);
    CHECK(c.holdings("Q12-B2") == 3);

    CHECK(b.spent() == Money::of(3870));

    // ---------------------------------------------------------
    // Q12 — Vendor information must remain in order history
    // ---------------------------------------------------------

    CHECK(acq.history().size() == 5);

    CHECK(
        acq.history()[0].vendor ==
        "Vendor B");

    CHECK(
        acq.history()[1].vendor ==
        "Vendor E");

    CHECK(
        acq.history()[2].vendor ==
        "Academic Supplier");

    CHECK(
        acq.history()[3].vendor ==
        "Vendor E");

    CHECK(
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

    CHECK(
        acq.quote("Q12-B3", 1) ==
        Money::of(250));

    PurchaseRecord noVendor =
        acq.purchase("Q12-B3", 1);

    CHECK(noVendor.approved);

    CHECK(noVendor.vendor.empty());

    CHECK(noVendor.cost == Money::of(250));

    CHECK(c.holdings("Q12-B3") == 1);

    // ---------------------------------------------------------
    // Q12 — Invalid vendor prices must be rejected
    // ---------------------------------------------------------

    CHECK_THROWS(
        acq.addVendor(
            "Q12-B1",
            "Invalid Vendor",
            Money::fromMinor(-1)),
        std::invalid_argument);

    // ---------------------------------------------------------
    // Q12 — Unknown resources cannot receive vendors
    // ---------------------------------------------------------

    CHECK_THROWS(
        acq.addVendor(
            "Q12-UNKNOWN",
            "Unknown Vendor",
            Money::of(100)),
        NotFoundError);

    CHECK_THROWS(
        acq.cheapestVendor("Q12-UNKNOWN"),
        NotFoundError);
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

    std::cout
        << (g_checks - g_failures)
        << "/" << g_checks
        << " checks passed";

    return g_failures == 0 ? 0 : 1;
}