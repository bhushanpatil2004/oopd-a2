#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "bookmgmt/Book.h"
#include "bookmgmt/ElectronicResource.h"
#include "bookmgmt/Exceptions.h"
#include "bookmgmt/Journal.h"

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

/*
 * Q12: Register a vendor offer for a resource.
 *
 * Multiple vendors may be registered for the same resource.
 * If the same vendor is registered again, its price is updated
 * instead of creating a duplicate offer.
 */
void AcquisitionManager::addVendor(
    const std::string& resourceId,
    const std::string& vendor,
    Money price) {

    if (!catalog_.find(resourceId)) {
        throw NotFoundError(resourceId);
    }

    if (vendor.empty()) {
        throw std::invalid_argument(
            "vendor name must not be empty");
    }

    if (price.isNegative()) {
        throw std::invalid_argument(
            "vendor price must not be negative");
    }

    auto& offers = vendorOffers_[resourceId];

    for (auto& offer : offers) {
        if (offer.vendor == vendor) {
            offer.price = price;
            return;
        }
    }

    offers.push_back(
        VendorOffer{vendor, price});
}

/*
 * Q12: Find the cheapest registered vendor.
 *
 * If two vendors have exactly the same price, the vendor that
 * was registered first is selected.
 */
const VendorOffer* AcquisitionManager::cheapestOffer(
    const std::string& resourceId) const {

    auto it = vendorOffers_.find(resourceId);

    if (it == vendorOffers_.end() ||
        it->second.empty()) {
        return nullptr;
    }

    const VendorOffer* cheapest =
        &it->second.front();

    for (const auto& offer : it->second) {
        if (offer.price < cheapest->price) {
            cheapest = &offer;
        }
    }

    return cheapest;
}

std::string AcquisitionManager::cheapestVendor(
    const std::string& resourceId) const {

    const VendorOffer* offer =
        cheapestOffer(resourceId);

    if (!offer) {
        throw NotFoundError(resourceId);
    }

    return offer->vendor;
}

/*
 * Q12: Calculate cost using a vendor's unit price.
 *
 * The normal Resource hierarchy remains unchanged. Vendor prices
 * temporarily replace the resource's unit price while preserving
 * resource-specific pricing rules:
 *
 * Book:
 *     vendor price * copies
 *     + 20% for Hardcover
 *
 * Journal:
 *     vendor price * copies * subscription years
 *
 * ElectronicResource:
 *     platform fee + vendor price * seats
 *
 * Thesis/other resources:
 *     vendor price * quantity
 *
 * Q5's bulk discount is applied separately in purchaseCost().
 */
Money AcquisitionManager::vendorCost(
    const Resource* r,
    int quantity,
    Money vendorPrice) const {

    if (quantity <= 0) {
        throw std::invalid_argument(
            "quantity must be positive");
    }

    switch (r->category()) {
        case ResourceCategory::Book: {
            Money cost = vendorPrice * quantity;

            const Book* book =
                dynamic_cast<const Book*>(r);

            if (book &&
                book->binding() == Binding::Hardcover) {
                cost = Money::fromMinor(
                    (cost.minorUnits() * 120) / 100);
            }

            return cost;
        }

        case ResourceCategory::Journal: {
            const Journal* journal =
                dynamic_cast<const Journal*>(r);

            if (journal) {
                return vendorPrice *
                       quantity *
                       journal->subscriptionYears();
            }

            return vendorPrice * quantity;
        }

        case ResourceCategory::ElectronicResource:
        case ResourceCategory::EBook:
        case ResourceCategory::AudioBook: {
            const ElectronicResource* electronic =
                dynamic_cast<const ElectronicResource*>(r);

            if (!electronic) {
                return vendorPrice * quantity;
            }

            Money cost =
                electronic->platformFee();

            if (quantity <= 50) {
                return cost +
                       vendorPrice * quantity;
            }

            cost += vendorPrice * 50;

            const int extraSeats =
                quantity - 50;

            Money halfPrice =
                Money::fromMinor(
                    vendorPrice.minorUnits() / 2);

            return cost +
                   halfPrice * extraSeats;
        }

        case ResourceCategory::Thesis:
            return vendorPrice * quantity;
    }

    return vendorPrice * quantity;
}

Money AcquisitionManager::purchaseCost(
    const Resource* r,
    int quantity) const {

    /*
     * Q12:
     * If vendors are registered, use the cheapest vendor's price.
     * Otherwise preserve the original Resource::costFor() behavior.
     */
    Money cost;

    const VendorOffer* offer =
        cheapestOffer(r->id());

    if (offer) {
        cost = vendorCost(
            r,
            quantity,
            offer->price);
    } else {
        cost = r->costFor(quantity);
    }

    /*
     * Q5: bulk discount for print resources.
     */
    if (quantity >= 10 &&
        (r->category() == ResourceCategory::Book ||
         r->category() == ResourceCategory::Journal ||
         r->category() == ResourceCategory::Thesis)) {

        return Money::fromMinor(
            (cost.minorUnits() * 90) / 100);
    }

    return cost;
}

Money AcquisitionManager::taxFor(
    const Resource* r,
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

Money AcquisitionManager::quote(
    const std::string& id,
    int quantity) const {

    const Resource& r =
        catalog_.get(id);

    const Money preTaxCost =
        purchaseCost(&r, quantity);

    return preTaxCost +
           taxFor(&r, preTaxCost);
}

Money AcquisitionManager::quote(
    const Department& department,
    const std::string& id,
    int quantity) const {

    // Check that the department exists.
    departmentBudget(department);

    const Resource& r =
        catalog_.get(id);

    const Money preTaxCost =
        purchaseCost(&r, quantity);

    return preTaxCost +
           taxFor(&r, preTaxCost);
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
        why = "department not found: " +
              department;
    } else if (!r) {
        why = "resource not found: " +
              id;
    } else if (quantity <= 0) {
        why = "quantity must be positive";
    } else {
        // Q7: title limit belongs to the department budget.
        why = budget->checkTitle(
            r->category(),
            r->id());

        if (why.empty()) {
            const Money preTaxCost =
                purchaseCost(
                    r,
                    quantity);

            const Money postTaxCost =
                preTaxCost +
                taxFor(r, preTaxCost);

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
    std::string reason,
    std::string vendor) {

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
        std::move(vendor),
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
        purchaseCost(
            &r,
            quantity);

    const Money tax =
        taxFor(
            &r,
            preTaxCost);

    const Money postTaxCost =
        preTaxCost +
        tax;

    const std::string reason =
        budget.check(
            r.category(),
            quantity,
            postTaxCost);

    if (!reason.empty()) {
        if (reason.find("quota") !=
            std::string::npos) {
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

    // Q12: record the cheapest vendor actually used.
    const VendorOffer* offer =
        cheapestOffer(r.id());

    const std::string vendor =
        offer ? offer->vendor : std::string{};

    return record(
        &r,
        department,
        id,
        quantity,
        preTaxCost,
        tax,
        postTaxCost,
        true,
        {},
        vendor);
}

std::vector<PurchaseRecord>
AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs,
    bool allOrNothing) {

    /*
     * Q11:
     *
     * In normal mode, preserve the existing sequential behaviour.
     *
     * In all-or-nothing mode, first validate the complete batch
     * against temporary copies of the affected department budgets.
     * Only after every request succeeds do we modify the real
     * budgets, catalogue holdings and order history.
     */

    if (!allOrNothing) {
        std::vector<PurchaseRecord> results;
        results.reserve(reqs.size());

        for (const auto& req : reqs) {

            const Budget* budget =
                findDepartmentBudget(
                    req.department);

            const Resource* r =
                catalog_.find(
                    req.resourceId);

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
                        preTaxCost +
                        tax;

                    why = budget->check(
                        r->category(),
                        req.quantity,
                        postTaxCost);
                }
            }

            if (why.empty()) {
                Budget& mutableBudget =
                    departmentBudget(
                        req.department);

                mutableBudget.commit(
                    r->category(),
                    req.quantity,
                    postTaxCost);

                mutableBudget.commitTitle(
                    r->category(),
                    r->id());

                catalog_.addHoldings(
                    req.resourceId,
                    req.quantity);

                // Q12: record cheapest vendor used.
                const VendorOffer* offer =
                    cheapestOffer(r->id());

                const std::string vendor =
                    offer
                        ? offer->vendor
                        : std::string{};

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
                        {},
                        vendor));
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

    /*
     * Q11 all-or-nothing mode.
     *
     * Make temporary copies of every department budget.
     * Budget contains all Q7/Q8/Q10 state, so this also simulates:
     *
     *   - spending
     *   - category unit usage
     *   - category spend usage
     *   - title limits
     *   - purchased-title tracking
     *
     * Q12 vendor selection is also performed during this validation.
     *
     * The real budgets are untouched during validation.
     */
    std::map<Department, Budget> temporaryBudgets;

    for (const auto& entry :
         departmentBudgets_) {

        temporaryBudgets.emplace(
            entry.first,
            *entry.second);
    }

    struct PendingPurchase {
        const PurchaseRequest* request;
        const Resource* resource;
        Money preTaxCost;
        Money tax;
        Money postTaxCost;
        std::string vendor;
    };

    std::vector<PendingPurchase> pending;
    pending.reserve(reqs.size());

    /*
     * Validate requests sequentially against the temporary budgets.
     * This is necessary because an earlier request can affect a later
     * request in the same batch.
     */
    for (const auto& req : reqs) {

        auto budgetIt =
            temporaryBudgets.find(
                req.department);

        const Resource* r =
            catalog_.find(
                req.resourceId);

        if (budgetIt ==
            temporaryBudgets.end()) {
            return {};
        }

        if (!r) {
            return {};
        }

        if (req.quantity <= 0) {
            return {};
        }

        Budget& temporaryBudget =
            budgetIt->second;

        std::string why =
            temporaryBudget.checkTitle(
                r->category(),
                r->id());

        if (!why.empty()) {
            return {};
        }

        const Money preTaxCost =
            purchaseCost(
                r,
                req.quantity);

        const Money tax =
            taxFor(
                r,
                preTaxCost);

        const Money postTaxCost =
            preTaxCost +
            tax;

        why = temporaryBudget.check(
            r->category(),
            req.quantity,
            postTaxCost);

        if (!why.empty()) {
            return {};
        }

        /*
         * Q12: determine the cheapest vendor during validation.
         * The selected vendor is stored in PendingPurchase so the
         * exact same vendor is recorded when the batch is committed.
         */
        const VendorOffer* offer =
            cheapestOffer(r->id());

        const std::string vendor =
            offer
                ? offer->vendor
                : std::string{};

        /*
         * Apply the request only to the temporary budget.
         * This makes subsequent requests see the updated
         * quota/title usage.
         */
        temporaryBudget.commit(
            r->category(),
            req.quantity,
            postTaxCost);

        temporaryBudget.commitTitle(
            r->category(),
            r->id());

        pending.push_back(
            PendingPurchase{
                &req,
                r,
                preTaxCost,
                tax,
                postTaxCost,
                vendor
            });
    }

    /*
     * Every request passed validation.
     * Now commit the entire batch to the real state.
     */
    std::vector<PurchaseRecord> results;
    results.reserve(pending.size());

    for (const auto& item : pending) {

        Budget& budget =
            departmentBudget(
                item.request->department);

        budget.commit(
            item.resource->category(),
            item.request->quantity,
            item.postTaxCost);

        budget.commitTitle(
            item.resource->category(),
            item.resource->id());

        catalog_.addHoldings(
            item.request->resourceId,
            item.request->quantity);

        results.push_back(
            record(
                item.resource,
                item.request->department,
                item.request->resourceId,
                item.request->quantity,
                item.preTaxCost,
                item.tax,
                item.postTaxCost,
                true,
                {},
                item.vendor));
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
        departmentBudget(
            original->department);

    // Check before changing any state.
    if (catalog_.holdings(
            original->resourceId) <
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
     *
     * Q12: cancellation records have no vendor because no
     * new vendor purchase is made.
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
        {},
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

        // Q12: show the vendor used for an approved purchase.
        if (!rec.vendor.empty()) {
            os << "  vendor: "
               << rec.vendor;
        }

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