#include "bookmgmt/Budget.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {
// Every category, in the order Budget::print() lists them.
const ResourceCategory kAllCategories[] = {
    ResourceCategory::Book,
    ResourceCategory::Journal,
    ResourceCategory::ElectronicResource,
    ResourceCategory::EBook,
    ResourceCategory::AudioBook,
    ResourceCategory::Thesis
};
} // namespace

Budget::Budget(Money total) : total_(total) {
    if (total_.isNegative())
        throw std::invalid_argument("budget must not be negative");
}

void Budget::setQuota(ResourceCategory c, Quota q) {
    if (q.maxUnits < 0 || q.maxSpend.isNegative())
        throw std::invalid_argument("quota limits must not be negative");

    quotas_[c] = q;
}

void Budget::removeQuota(ResourceCategory c) {
    quotas_.erase(c);
}

std::optional<Quota> Budget::quotaFor(ResourceCategory c) const {
    auto it = quotas_.find(c);

    if (it == quotas_.end())
        return std::nullopt;

    return it->second;
}

Usage Budget::usageFor(ResourceCategory c) const {
    auto it = usage_.find(c);

    return it == usage_.end() ? Usage{} : it->second;
}

std::optional<int> Budget::unitsRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);

    if (!q)
        return std::nullopt;

    return q->maxUnits - usageFor(c).units;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);

    if (!q)
        return std::nullopt;

    return q->maxSpend - usageFor(c).spent;
}

Budget::Failure Budget::evaluate(ResourceCategory c,
                                 int units,
                                 Money cost,
                                 std::string& why) const {
    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }

    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }

    if (auto left = unitsRemaining(c); left && units > *left) {
        why = std::string(categoryName(c)) +
              " unit quota exceeded: requested " +
              std::to_string(units) +
              ", " +
              std::to_string(*left) +
              " remaining";

        return Failure::Quota;
    }

    if (auto left = spendRemaining(c); left && cost > *left) {
        why = std::string(categoryName(c)) +
              " spend quota exceeded: cost " +
              cost.toString() +
              ", " +
              left->toString() +
              " remaining";

        return Failure::Quota;
    }

    if (cost > remaining()) {
        why = "overall budget exceeded: cost " +
              cost.toString() +
              ", " +
              remaining().toString() +
              " remaining";

        return Failure::Overall;
    }

    why.clear();
    return Failure::None;
}

std::string Budget::check(ResourceCategory c,
                          int units,
                          Money cost) const {
    std::string why;

    evaluate(c, units, cost, why);

    return why;
}

void Budget::commit(ResourceCategory c,
                    int units,
                    Money cost) {
    std::string why;

    switch (evaluate(c, units, cost, why)) {
        case Failure::None:
            break;

        case Failure::BadInput:
            throw std::invalid_argument(why);

        case Failure::Quota:
            throw QuotaExceededError(why);

        case Failure::Overall:
            throw BudgetExceededError(why);
    }

    Usage& u = usage_[c];

    u.units += units;
    u.spent += cost;
    spent_ += cost;
}

/*
 * Q7: Limit the number of different titles that may be bought
 * in each category.
 */
void Budget::setTitleLimit(ResourceCategory c, int maxTitles) {
    if (maxTitles < 0)
        throw std::invalid_argument(
            "title limit must not be negative");

    titleLimits_[c] = maxTitles;
}

std::optional<int> Budget::titleLimitFor(ResourceCategory c) const {
    auto it = titleLimits_.find(c);

    if (it == titleLimits_.end())
        return std::nullopt;

    return it->second;
}

int Budget::titlesUsed(ResourceCategory c) const {
    auto it = purchasedTitles_.find(c);

    if (it == purchasedTitles_.end())
        return 0;

    return static_cast<int>(it->second.size());
}

std::string Budget::checkTitle(
    ResourceCategory c,
    const std::string& resourceId) const {

    if (resourceId.empty())
        return "resource id must not be empty";

    auto titlesIt = purchasedTitles_.find(c);

    // Buying more copies/seats of an already purchased title
    // does not consume another title slot.
    if (titlesIt != purchasedTitles_.end() &&
        titlesIt->second.find(resourceId) != titlesIt->second.end()) {
        return {};
    }

    auto limitIt = titleLimits_.find(c);

    // No title limit configured for this category.
    if (limitIt == titleLimits_.end())
        return {};

    if (titlesUsed(c) >= limitIt->second) {
        return std::string(categoryName(c)) +
               " different-title limit exceeded";
    }

    return {};
}

void Budget::commitTitle(ResourceCategory c,
                         const std::string& resourceId) {
    if (resourceId.empty())
        throw std::invalid_argument(
            "resource id must not be empty");

    auto& titles = purchasedTitles_[c];

    // The same title can be bought again without increasing
    // the number of different titles.
    if (titles.find(resourceId) != titles.end())
        return;

    auto limitIt = titleLimits_.find(c);

    if (limitIt != titleLimits_.end() &&
        static_cast<int>(titles.size()) >= limitIt->second) {
        throw QuotaExceededError(
            std::string(categoryName(c)) +
            " different-title limit exceeded");
    }

    titles.insert(resourceId);
}

/*
 * Q8: Refund a previously approved purchase.
 *
 * This reverses exactly the category usage and total spending that
 * were added by Budget::commit().
 */
void Budget::refund(ResourceCategory c,
                    int units,
                    Money cost) {
    if (units <= 0)
        throw std::invalid_argument(
            "refund quantity must be positive");

    if (cost.isNegative())
        throw std::invalid_argument(
            "refund cost must not be negative");

    Usage& u = usage_[c];

    if (units > u.units)
        throw std::invalid_argument(
            "refund exceeds category unit usage");

    if (cost > u.spent)
        throw std::invalid_argument(
            "refund exceeds category spend usage");

    if (cost > spent_)
        throw std::invalid_argument(
            "refund exceeds total budget usage");

    u.units -= units;
    u.spent -= cost;
    spent_ -= cost;
}

/*
 * Q8: Release a title from Q7 tracking.
 *
 * AcquisitionManager calls this only after confirming that no
 * remaining approved purchase still uses the title.
 */
void Budget::releaseTitle(
    ResourceCategory c,
    const std::string& resourceId) {
    if (resourceId.empty())
        throw std::invalid_argument(
            "resource id must not be empty");

    auto it = purchasedTitles_.find(c);

    if (it == purchasedTitles_.end())
        return;

    it->second.erase(resourceId);
}

/*
 * Q10: Year-end rollover.
 *
 * The next year's budget receives a configurable percentage of the
 * current year's unspent amount.
 *
 * Example:
 *     Current total  = 10000
 *     Current spent  = 6000
 *     Unspent        = 4000
 *     Rollover       = 50%
 *     New total      = 2000
 *
 * Quota configuration is carried forward, but actual usage and
 * purchased-title history are reset because the new budget belongs
 * to a new financial year.
 */
Budget Budget::rollover(int percentage) const {
    if (percentage < 0 || percentage > 100) {
        throw std::invalid_argument(
            "rollover percentage must be between 0 and 100");
    }

    const Money unspent = remaining();

    const Money carriedForward =
        Money::fromMinor(
            (unspent.minorUnits() * percentage) / 100);

    Budget nextYear(carriedForward);

    // Carry forward the configured category quotas.
    nextYear.quotas_ = quotas_;

    // Carry forward Q7 title-limit configuration.
    // Do NOT carry forward purchased title history.
    nextYear.titleLimits_ = titleLimits_;

    // nextYear.usage_ is intentionally empty.
    // nextYear.purchasedTitles_ is intentionally empty.
    // nextYear.spent_ is already zero.

    return nextYear;
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total "
       << total_
       << ", spent "
       << spent_
       << ", remaining "
       << remaining()
       << "\n";

    os << std::left
       << std::setw(22)
       << "  Category"
       << std::setw(18)
       << "Units used/max"
       << "Spend used/max\n";

    for (ResourceCategory c : kAllCategories) {
        const Usage u = usageFor(c);
        const auto q = quotaFor(c);

        const std::string units =
            std::to_string(u.units) +
            "/" +
            (q ? std::to_string(q->maxUnits) : "-");

        const std::string spend =
            u.spent.toString() +
            "/" +
            (q ? q->maxSpend.toString() : "-");

        os << "  "
           << std::setw(20)
           << categoryName(c)
           << std::setw(18)
           << units
           << spend
           << "\n";
    }
}

}  // namespace bookmgmt