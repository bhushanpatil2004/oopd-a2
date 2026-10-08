#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog,
                                       Budget& budget,
                                       int printTaxPercent,
                                       int electronicTaxPercent)
    : catalog_(catalog),
      defaultBudget_(budget),
      printTaxPercent_(0),
      electronicTaxPercent_(0) {
    // The original budget is the default department budget.
    departmentBudgets_["Default"] = &defaultBudget_;

    setTaxRates(printTaxPercent, electronicTaxPercent);
}

void AcquisitionManager::setTaxRates(int printTaxPercent,
                                      int electronicTaxPercent) {
    if (printTaxPercent < 0 || printTaxPercent > 100 ||
        electronicTaxPercent < 0 || electronicTaxPercent > 100) {
        throw std::invalid_argument(
            "tax rates must be between 0 and 100");
    }

    printTaxPercent_ = printTaxPercent;
    electronicTaxPercent_ = electronicTaxPercent;
}

void AcquisitionManager::addDepartment(
    const Department& department,
    Budget& budget) {

    if (department.empty()) {
        throw std::invalid_argument(
            "department name must not be empty");
    }

    departmentBudgets_[department] = &budget;
}

Budget* AcquisitionManager::findDepartmentBudget(
    const Department& department) {

    auto it = departmentBudgets_.find(department);

    if (it == departmentBudgets_.end()) {
        return nullptr;
    }

    return it->second;
}

const Budget* AcquisitionManager::findDepartmentBudget(
    const Department& department) const {

    auto it = departmentBudgets_.find(department);

    if (it == departmentBudgets_.end()) {
        return nullptr;
    }

    return it->second;
}

Budget& AcquisitionManager::departmentBudget(
    const Department& department) {

    Budget* budget = findDepartmentBudget(department);

    if (!budget) {
        throw NotFoundError(department);
    }

    return *budget;
}

const Budget& AcquisitionManager::departmentBudget(
    const Department& department) const {

    const Budget* budget = findDepartmentBudget(department);

    if (!budget) {
        throw NotFoundError(department);
    }

    return *budget;
}

Money AcquisitionManager::purchaseCost(const Resource* r,
                                       int quantity) const {
    Money cost = r->costFor(quantity);

    if (quantity >= 10 &&
        (r->category() == ResourceCategory::Book ||
         r->category() == ResourceCategory::Journal ||
         r->category() == ResourceCategory::Thesis)) {
        return Money::fromMinor((cost.minorUnits() * 90) / 100);
    }

    return cost;
}

Money AcquisitionManager::taxFor(const Resource* r,
                                 Money preTaxCost) const {
    if (!r) {
        return Money{};
    }

    int rate = 0;

    switch (r->category()) {
        case ResourceCategory::Book:
        case ResourceCategory::Journal:
        case ResourceCategory::Thesis:
            rate = printTaxPercent_;
            break;

        case ResourceCategory::ElectronicResource:
        case ResourceCategory::EBook:
        case ResourceCategory::AudioBook:
            rate = electronicTaxPercent_;
            break;
    }

    return Money::fromMinor(
        (preTaxCost.minorUnits() * rate) / 100);
}

Money AcquisitionManager::quote(const std::string& id,
                                 int quantity) const {
    const Resource& r = catalog_.get(id);

    const Money preTaxCost =
        purchaseCost(&r, quantity);

    return preTaxCost + taxFor(&r, preTaxCost);
}

Money AcquisitionManager::quote(
    const Department& department,
    const std::string& id,
    int quantity) const {

    // Check that the department exists.
    departmentBudget(department);

    const Resource& r = catalog_.get(id);

    const Money preTaxCost =
        purchaseCost(&r, quantity);

    return preTaxCost + taxFor(&r, preTaxCost);
}

bool AcquisitionManager::canPurchase(
    const std::string& id,
    int quantity,
    std::string* reason) const {

    return canPurchase(
        "Default",
        id,
        quantity,
        reason);
}

bool AcquisitionManager::canPurchase(
    const Department& department,
    const std::string& id,
    int quantity,
    std::string* reason) const {

    std::string why;

    const Budget* budget =
        findDepartmentBudget(department);

    const Resource* r =
        catalog_.find(id);

    if (!budget) {
        why = "department not found: " + department;
    } else if (!r) {
        why = "resource not found: " + id;
    } else if (quantity <= 0) {
        why = "quantity must be positive";
    } else {
        // Q7: title limit belongs to the department budget.
        why = budget->checkTitle(
            r->category(),
            r->id());

        if (why.empty()) {
            const Money preTaxCost =
                purchaseCost(r, quantity);

            const Money postTaxCost =
                preTaxCost + taxFor(r, preTaxCost);

            why = budget->check(
                r->category(),
                quantity,
                postTaxCost);
        }
    }

    if (reason) {
        *reason = why;
    }

    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(
    const Resource* r,
    const Department& department,
    const std::string& id,
    int qty,
    Money preTaxCost,
    Money tax,
    Money cost,
    bool approved,
    std::string reason) {

    history_.push_back(PurchaseRecord{
        nextOrderNo_++,
        department,
        id,
        r ? r->title() : std::string("(unknown)"),
        r ? r->category() : ResourceCategory::Book,
        qty,
        preTaxCost,
        tax,
        cost,
        approved,
        std::move(reason),
        false
    });

    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(
    const std::string& id,
    int quantity) {

    return purchase(
        "Default",
        id,
        quantity);
}

const PurchaseRecord& AcquisitionManager::purchase(
    const Department& department,
    const std::string& id,
    int quantity) {

    Budget& budget =
        departmentBudget(department);

    const Resource& r =
        catalog_.get(id);

    if (quantity <= 0) {
        throw std::invalid_argument(
            "quantity must be positive");
    }

    // Q7: title limit is checked before normal budget quotas.
    const std::string titleReason =
        budget.checkTitle(
            r.category(),
            r.id());

    if (!titleReason.empty()) {
        throw QuotaExceededError(titleReason);
    }

    const Money preTaxCost =
        purchaseCost(&r, quantity);

    const Money tax =
        taxFor(&r, preTaxCost);

    const Money postTaxCost =
        preTaxCost + tax;

    const std::string reason =
        budget.check(
            r.category(),
            quantity,
            postTaxCost);

    if (!reason.empty()) {
        if (reason.find("quota") != std::string::npos) {
            throw QuotaExceededError(reason);
        }

        throw BudgetExceededError(reason);
    }

    budget.commit(
        r.category(),
        quantity,
        postTaxCost);

    // Q7: record the title only after the complete purchase succeeds.
    budget.commitTitle(
        r.category(),
        r.id());

    catalog_.addHoldings(
        id,
        quantity);

    return record(
        &r,
        department,
        id,
        quantity,
        preTaxCost,
        tax,
        postTaxCost,
        true,
        {});
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs) {

    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());

    for (const auto& req : reqs) {

        const Budget* budget =
            findDepartmentBudget(req.department);

        const Resource* r =
            catalog_.find(req.resourceId);

        Money preTaxCost;
        Money tax;
        Money postTaxCost;
        std::string why;

        if (!budget) {
            why = "department not found: " +
                  req.department;
        } else if (!r) {
            why = "resource not found: " +
                  req.resourceId;
        } else if (req.quantity <= 0) {
            why = "quantity must be positive";
        } else {
            // Q7: title limit is checked before spending quotas.
            why = budget->checkTitle(
                r->category(),
                r->id());

            if (why.empty()) {
                preTaxCost =
                    purchaseCost(
                        r,
                        req.quantity);

                tax =
                    taxFor(
                        r,
                        preTaxCost);

                postTaxCost =
                    preTaxCost + tax;

                why = budget->check(
                    r->category(),
                    req.quantity,
                    postTaxCost);
            }
        }

        if (why.empty()) {
            Budget& mutableBudget =
                departmentBudget(req.department);

            mutableBudget.commit(
                r->category(),
                req.quantity,
                postTaxCost);

            // Record the title only for an approved purchase.
            mutableBudget.commitTitle(
                r->category(),
                r->id());

            catalog_.addHoldings(
                req.resourceId,
                req.quantity);

            results.push_back(
                record(
                    r,
                    req.department,
                    req.resourceId,
                    req.quantity,
                    preTaxCost,
                    tax,
                    postTaxCost,
                    true,
                    {}));
        } else {
            results.push_back(
                record(
                    r,
                    req.department,
                    req.resourceId,
                    req.quantity,
                    preTaxCost,
                    tax,
                    postTaxCost,
                    false,
                    why));
        }
    }

    return results;
}

bool AcquisitionManager::hasActivePurchase(
    const std::string& resourceId,
    int excludedOrderNo) const {

    for (const auto& rec : history_) {
        if (rec.cancellation) {
            continue;
        }

        if (!rec.approved) {
            continue;
        }

        if (rec.orderNo == excludedOrderNo) {
            continue;
        }

        if (cancelledOrders_.find(rec.orderNo) !=
            cancelledOrders_.end()) {
            continue;
        }

        if (rec.resourceId == resourceId) {
            return true;
        }
    }

    return false;
}

const PurchaseRecord& AcquisitionManager::cancel(
    int orderNo) {

    PurchaseRecord* original = nullptr;

    for (auto& rec : history_) {
        if (rec.orderNo == orderNo) {
            original = &rec;
            break;
        }
    }

    if (!original) {
        throw NotFoundError(
            std::to_string(orderNo));
    }

    if (original->cancellation) {
        throw std::invalid_argument(
            "cannot cancel a cancellation record");
    }

    if (!original->approved) {
        throw std::invalid_argument(
            "cannot cancel a rejected order");
    }

    if (cancelledOrders_.find(orderNo) !=
        cancelledOrders_.end()) {
        throw std::invalid_argument(
            "order already cancelled");
    }

    Budget& budget =
        departmentBudget(original->department);

    // Check before changing any state.
    if (catalog_.holdings(original->resourceId) <
        original->quantity) {
        throw std::invalid_argument(
            "cannot cancel order: insufficient holdings");
    }

    /*
     * Refund exactly the post-tax amount that was originally charged.
     */
    budget.refund(
        original->category,
        original->quantity,
        original->cost);

    // Reduce catalog holdings.
    catalog_.addHoldings(
        original->resourceId,
        -original->quantity);

    /*
     * A title slot is released only when no other active purchase
     * still uses this title in the department.
     */
    bool departmentStillUsesTitle = false;

    for (const auto& rec : history_) {
        if (rec.cancellation ||
            !rec.approved ||
            rec.orderNo == original->orderNo ||
            cancelledOrders_.find(rec.orderNo) !=
                cancelledOrders_.end()) {
            continue;
        }

        if (rec.department == original->department &&
            rec.resourceId == original->resourceId) {
            departmentStillUsesTitle = true;
            break;
        }
    }

    if (!departmentStillUsesTitle) {
        budget.releaseTitle(
            original->category,
            original->resourceId);
    }

    // Mark the original order as cancelled.
    cancelledOrders_.insert(orderNo);

    /*
     * Keep the original record unchanged and add a separate
     * cancellation record.
     */
    history_.push_back(PurchaseRecord{
        nextOrderNo_++,
        original->department,
        original->resourceId,
        original->title,
        original->category,
        original->quantity,
        Money{},
        Money{},
        Money{},
        true,
        "cancelled order #" +
            std::to_string(orderNo),
        true
    });

    return history_.back();
}

Money AcquisitionManager::totalSpent() const {
    Money sum;

    for (const auto& rec : history_) {
        if (rec.approved &&
            !rec.cancellation &&
            cancelledOrders_.find(rec.orderNo) ==
                cancelledOrders_.end()) {

            sum += rec.cost;
        }
    }

    return sum;
}

void AcquisitionManager::printReport(
    std::ostream& os) const {

    Money totalPreTax;
    Money totalTax;
    Money totalPostTax;

    os << "Order history (" << history_.size()
       << " orders)\n";

    for (const auto& rec : history_) {

        const char* status =
            rec.cancellation
                ? "CANCELLED"
                : (rec.approved
                       ? "APPROVED"
                       : "REJECTED");

        os << "  #" << std::setw(3)
           << std::left
           << rec.orderNo
           << " "
           << status
           << "  "
           << std::setw(18)
           << rec.department
           << " "
           << std::setw(6)
           << rec.resourceId
           << " x"
           << std::setw(3)
           << rec.quantity
           << " pre-tax "
           << std::setw(12)
           << std::right
           << rec.preTaxCost.toString()
           << " tax "
           << std::setw(10)
           << rec.tax.toString()
           << " post-tax "
           << std::setw(12)
           << rec.cost.toString()
           << std::left
           << "  "
           << rec.title;

        if (!rec.approved ||
            rec.cancellation) {
            os << "\n        reason: "
               << rec.reason;
        }

        os << "\n";

        /*
         * Cancelled original purchases must no longer contribute
         * to the active totals.
         */
        if (rec.approved &&
            !rec.cancellation &&
            cancelledOrders_.find(rec.orderNo) ==
                cancelledOrders_.end()) {

            totalPreTax += rec.preTaxCost;
            totalTax += rec.tax;
            totalPostTax += rec.cost;
        }
    }

    os << "Pre-tax total: "
       << totalPreTax << "\n";

    os << "Tax total: "
       << totalTax << "\n";

    os << "Post-tax total: "
       << totalPostTax << "\n";

    os << "Total spent: "
       << totalSpent() << "\n";
}

}  // namespace bookmgmt