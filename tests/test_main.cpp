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
    Journal j("J1","ACM Computing Surveys","0360-0300",4,"ACM",2026,Money::of(500),3);
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

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
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

    CHECK_THROWS(Book("", "T", {}, "", "", 2000, Money::of(1)), std::invalid_argument);
    CHECK_THROWS(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)),
                 std::invalid_argument);
}

static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    CHECK(c.size() == 3);
    CHECK(c.contains("B1"));
    CHECK(c.find("nope") == nullptr);
    CHECK_THROWS(c.get("nope"), NotFoundError);
    CHECK_THROWS(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)),
                 DuplicateIdError);

    CHECK(c.searchTitle("clean").size() == 2);
    CHECK(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    CHECK(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);

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
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});
    b.setQuota(ResourceCategory::Journal, {4, Money::of(3000)});
    CHECK(b.check(ResourceCategory::Journal, 3, Money::of(800)).empty());

    CHECK(!b.check(ResourceCategory::Journal, 5, Money::of(100)).empty());

    CHECK(!b.check(ResourceCategory::Journal, 1, Money::of(3001)).empty());

    CHECK(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    CHECK(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    CHECK(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    CHECK(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    CHECK(b.spent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    CHECK_THROWS(b.commit(ResourceCategory::Book, 2, Money::of(10)), QuotaExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)),
                 BudgetExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)),
                 std::invalid_argument);
    CHECK(b.spent() == Money::of(300));  // failed commits changed nothing

    Budget ebookBudget(Money::of(25000));

    ebookBudget.setQuota(
        ResourceCategory::EBook,
        {5, Money::of(2500)});

    CHECK(ebookBudget.check(
        ResourceCategory::EBook,
        3,
        Money::of(1700)).empty());

    CHECK(!ebookBudget.check(
        ResourceCategory::EBook,
        6,
        Money::of(1700)).empty());

    CHECK(!ebookBudget.check(
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
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    CHECK(acq.canPurchase("B1", 3, &why) && why.empty());
    CHECK(!acq.canPurchase("B1", 4, &why) && !why.empty());
    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    CHECK(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    CHECK(c.holdings("B1") == 2);

    CHECK_THROWS(acq.purchase("B1", 2), QuotaExceededError);
    CHECK_THROWS(acq.purchase("nope", 1), NotFoundError);
    CHECK(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    CHECK(res.size() == 5);
    CHECK(res[0].approved && res[0].cost == Money::of(150));
    CHECK(!res[1].approved);  // 1050 > remaining 150
    CHECK(res[2].approved);
    CHECK(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
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

    // Configurable tax rates.
    CHECK(acq.printTaxPercent() == 10);
    CHECK(acq.electronicTaxPercent() == 20);

    // Quotes include tax.
    CHECK(acq.quote("Q6-B", 1) == Money::of(110));
    CHECK(acq.quote("Q6-E", 1) == Money::of(120));

    // Purchase record stores pre-tax, tax and post-tax amounts.
    const auto& bookRecord = acq.purchase("Q6-B", 1);

    CHECK(bookRecord.preTaxCost == Money::of(100));
    CHECK(bookRecord.tax == Money::of(10));
    CHECK(bookRecord.cost == Money::of(110));

    // Budget stores the post-tax amount.
    CHECK(b.spent() == Money::of(110));
    CHECK(b.usageFor(ResourceCategory::Book).spent == Money::of(110));

    // Tax rates can be changed.
    acq.setTaxRates(5, 15);

    CHECK(acq.printTaxPercent() == 5);
    CHECK(acq.electronicTaxPercent() == 15);

    CHECK(acq.quote("Q6-B", 1) == Money::of(105));
    CHECK(acq.quote("Q6-E", 1) == Money::of(115));

    // Invalid tax rates are rejected.
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

    // Quota is deliberately below the post-tax price.
    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(105)});

    // Book tax = 10%.
    AcquisitionManager acq(c, b, 10, 0);

    // Pre-tax = 100, tax = 10, post-tax = 110.
    CHECK(acq.quote("Q6-Q", 1) == Money::of(110));

    std::string reason;

    // 110 > quota of 105, so this must be rejected.
    CHECK(!acq.canPurchase("Q6-Q", 1, &reason));
    CHECK(!reason.empty());

    // Budget must remain unchanged after the rejected check.
    CHECK(b.spent() == Money{});
}
int main() {
    testMoney();
    testResourcesAndCost();
    testCatalog();
    testBudget();
    testAcquisition();
    testTaxes();
    testPostTaxQuota();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
