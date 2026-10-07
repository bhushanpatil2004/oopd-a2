#pragma once
// Budget: an overall spending limit plus optional per-category purchase quotas.
//
// A quota caps how many units (copies/seats) and how much money may be spent on
// one category. Categories with no quota are limited only by the total budget.

#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <string>

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt {

struct Quota {
    int maxUnits;    // maximum copies/seats that may be bought
    Money maxSpend;  // maximum money that may be spent on one category
};

struct Usage {
    int units = 0;
    Money spent;
};

class Budget {
public:
    explicit Budget(Money total);

    Money total() const { return total_; }
    Money spent() const { return spent_; }
    Money remaining() const { return total_ - spent_; }

    void setQuota(ResourceCategory c, Quota q);
    void removeQuota(ResourceCategory c);
    std::optional<Quota> quotaFor(ResourceCategory c) const;
    Usage usageFor(ResourceCategory c) const;

    // Remaining allowance in a category; nullopt means "no quota set".
    std::optional<int> unitsRemaining(ResourceCategory c) const;
    std::optional<Money> spendRemaining(ResourceCategory c) const;

    // Returns an empty string if the purchase fits, otherwise the reason it
    // does not. Does not change state.
    std::string check(ResourceCategory c, int units, Money cost) const;

    // Records a purchase. Throws QuotaExceededError / BudgetExceededError
    // (and changes nothing) if it would not fit.
    void commit(ResourceCategory c, int units, Money cost);

    // Q7: limits the number of different titles that may be bought
    // in a category.
    void setTitleLimit(ResourceCategory c, int maxTitles);

    // Returns the configured title limit. nullopt means no title limit.
    std::optional<int> titleLimitFor(ResourceCategory c) const;

    // Returns the number of different titles already purchased
    // in a category.
    int titlesUsed(ResourceCategory c) const;

    // Returns an empty string if the title can be purchased, otherwise
    // returns the title-limit failure reason. Does not change state.
    std::string checkTitle(ResourceCategory c,
                           const std::string& resourceId) const;

    // Records a newly purchased title.
    // Does nothing if the title has already been recorded.
    void commitTitle(ResourceCategory c,
                     const std::string& resourceId);

    void print(std::ostream& os) const;

private:
    enum class Failure { None, BadInput, Quota, Overall };
    Failure evaluate(ResourceCategory c, int units, Money cost, std::string& why) const;

    Money total_;
    Money spent_;
    std::map<ResourceCategory, Quota> quotas_;
    std::map<ResourceCategory, Usage> usage_;

    // Q7: maximum number of different titles allowed in each category.
    std::map<ResourceCategory, int> titleLimits_;

    // Q7: resource IDs of titles already purchased in each category.
    std::map<ResourceCategory, std::set<std::string>> purchasedTitles_;
};

}  // namespace bookmgmt