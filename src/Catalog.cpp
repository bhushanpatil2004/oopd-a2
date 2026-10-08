#include "bookmgmt/Catalog.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "bookmgmt/Book.h"
#include "bookmgmt/EBook.h"
#include "bookmgmt/Journal.h"
#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return s;
}

}  // namespace

Resource& Catalog::add(std::unique_ptr<Resource> r) {
    if (!r) throw std::invalid_argument("cannot add a null resource");

    const std::string id = r->id();

    if (items_.count(id)) throw DuplicateIdError(id);

    Entry& e = items_[id];
    e.resource = std::move(r);

    return *e.resource;
}

bool Catalog::contains(const std::string& id) const {
    return items_.count(id) > 0;
}

Resource* Catalog::find(const std::string& id) {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

const Resource* Catalog::find(const std::string& id) const {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

Resource& Catalog::get(const std::string& id) {
    if (Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

const Resource& Catalog::get(const std::string& id) const {
    if (const Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

void Catalog::remove(const std::string& id) {
    if (items_.erase(id) == 0) throw NotFoundError(id);
}

int Catalog::holdings(const std::string& id) const {
    auto it = items_.find(id);

    if (it == items_.end()) throw NotFoundError(id);

    return it->second.holdings;
}

void Catalog::addHoldings(const std::string& id, int units) {
    auto it = items_.find(id);

    if (it == items_.end()) throw NotFoundError(id);

    if (it->second.holdings + units < 0)
        throw std::invalid_argument("holdings cannot become negative");

    it->second.holdings += units;
}

std::vector<const Resource*> Catalog::where(
    const std::function<bool(const Resource&)>& pred) const {

    std::vector<const Resource*> out;

    for (const auto& [id, entry] : items_)
        if (pred(*entry.resource))
            out.push_back(entry.resource.get());

    return out;
}

std::vector<const Resource*> Catalog::all() const {
    return where([](const Resource&) {
        return true;
    });
}

std::vector<const Resource*> Catalog::byCategory(
    ResourceCategory c) const {

    return where([c](const Resource& r) {
        return r.category() == c;
    });
}

std::vector<const Resource*> Catalog::searchTitle(
    const std::string& text) const {

    const std::string needle = lower(text);

    return where([&needle](const Resource& r) {
        return lower(r.title()).find(needle) != std::string::npos;
    });
}

// ============================================================
// Q13 — Search by Author
// ============================================================

std::vector<const Resource*> Catalog::searchAuthor(
    const std::string& author) const {

    const std::string needle = lower(author);

    return where([&needle](const Resource& r) {

        if (const auto* book = dynamic_cast<const Book*>(&r)) {
            for (const std::string& name : book->authors()) {
                if (lower(name).find(needle) != std::string::npos)
                    return true;
            }
        }

        if (const auto* ebook = dynamic_cast<const EBook*>(&r)) {
            for (const std::string& name : ebook->authors()) {
                if (lower(name).find(needle) != std::string::npos)
                    return true;
            }
        }

        return false;
    });
}

// ============================================================
// Q13 — Search by ISBN / ISSN
// ============================================================

std::vector<const Resource*> Catalog::searchIsbnIssn(
    const std::string& identifier) const {

    const std::string needle = lower(identifier);

    return where([&needle](const Resource& r) {

        if (r.category() == ResourceCategory::Book) {
            const auto* book = dynamic_cast<const Book*>(&r);

            return book != nullptr &&
                   lower(book->isbn()) == needle;
        }

        if (r.category() == ResourceCategory::EBook) {
            const auto* ebook = dynamic_cast<const EBook*>(&r);

            return ebook != nullptr &&
                   lower(ebook->isbn()) == needle;
        }

        if (r.category() == ResourceCategory::Journal) {
            const auto* journal = dynamic_cast<const Journal*>(&r);

            return journal != nullptr &&
                   lower(journal->issn()) == needle;
        }

        return false;
    });
}

// ============================================================
// Q13 — Search by Publication Year Range
// ============================================================

std::vector<const Resource*> Catalog::searchYearRange(
    int fromYear,
    int toYear) const {

    if (fromYear > toYear) {
        throw std::invalid_argument(
            "fromYear cannot be greater than toYear");
    }

    return where([fromYear, toYear](const Resource& r) {
        return r.year() >= fromYear &&
               r.year() <= toYear;
    });
}

}  // namespace bookmgmt