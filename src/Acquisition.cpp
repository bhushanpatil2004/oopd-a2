#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget,
                                       int printTaxPercent,
                                       int electronicTaxPercent)
    : catalog_(catalog),
      budget_(budget),
      printTaxPercent_(0),
      electronicTaxPercent_(0) {
    setTaxRates(printTaxPercent, electronicTaxPercent);
}

void AcquisitionManager::setTaxRates(int printTaxPercent,
                                      int electronicTaxPercent) {
    if (printTaxPercent < 0 || printTaxPercent > 100 ||
        electronicTaxPercent < 0 || electronicTaxPercent > 100) {
        throw std::invalid_argument("tax rates must be between 0 and 100");
    }

    printTaxPercent_ = printTaxPercent;
    electronicTaxPercent_ = electronicTaxPercent;
}

Money AcquisitionManager::purchaseCost(const Resource* r, int quantity) const {
    Money cost = r->costFor(quantity);

    if (quantity >= 10 &&
        (r->category() == ResourceCategory::Book ||
         r->category() == ResourceCategory::Journal ||
         r->category() == ResourceCategory::Thesis)) {
        return Money::fromMinor((cost.minorUnits() * 90) / 100);
    }

    return cost;
}

Money AcquisitionManager::taxFor(const Resource* r, Money preTaxCost) const {
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

Money AcquisitionManager::quote(const std::string& id, int quantity) const {
    const Resource* r = &catalog_.get(id);
    const Money preTaxCost = purchaseCost(r, quantity);
    return preTaxCost + taxFor(r, preTaxCost);
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                      std::string* reason) const {
    std::string why;

    if (const Resource* r = catalog_.find(id)) {
        if (quantity <= 0) {
            why = "quantity must be positive";
        } else {
            const Money preTaxCost = purchaseCost(r, quantity);
            const Money postTaxCost = preTaxCost + taxFor(r, preTaxCost);

            why = budget_.check(
                r->category(),
                quantity,
                postTaxCost);
        }
    } else {
        why = "resource not found: " + id;
    }

    if (reason) {
        *reason = why;
    }

    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(const Resource* r,
                                           const std::string& id,
                                           int qty,
                                           Money preTaxCost,
                                           Money tax,
                                           Money cost,
                                           bool approved,
                                           std::string reason) {
    history_.push_back(PurchaseRecord{
        nextOrderNo_++,
        id,
        r ? r->title() : std::string("(unknown)"),
        r ? r->category() : ResourceCategory::Book,
        qty,
        preTaxCost,
        tax,
        cost,
        approved,
        std::move(reason)});

    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id,
                                                   int quantity) {
    const Resource& r = catalog_.get(id);

    if (quantity <= 0) {
        throw std::invalid_argument("quantity must be positive");
    }

    const Money preTaxCost = purchaseCost(&r, quantity);
    const Money tax = taxFor(&r, preTaxCost);
    const Money postTaxCost = preTaxCost + tax;

    const std::string reason =
        budget_.check(r.category(), quantity, postTaxCost);

    if (!reason.empty()) {
        if (reason.find("quota") != std::string::npos) {
            throw QuotaExceededError(reason);
        }

        throw BudgetExceededError(reason);
    }

    budget_.commit(r.category(), quantity, postTaxCost);
    catalog_.addHoldings(id, quantity);

    return record(&r,
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
        const Resource* r = catalog_.find(req.resourceId);

        Money preTaxCost;
        Money tax;
        Money postTaxCost;
        std::string why;

        if (!r) {
            why = "resource not found: " + req.resourceId;
        } else if (req.quantity <= 0) {
            why = "quantity must be positive";
        } else {
            preTaxCost = purchaseCost(r, req.quantity);
            tax = taxFor(r, preTaxCost);
            postTaxCost = preTaxCost + tax;

            why = budget_.check(
                r->category(),
                req.quantity,
                postTaxCost);
        }

        if (why.empty()) {
            budget_.commit(
                r->category(),
                req.quantity,
                postTaxCost);

            catalog_.addHoldings(
                req.resourceId,
                req.quantity);

            results.push_back(
                record(r,
                       req.resourceId,
                       req.quantity,
                       preTaxCost,
                       tax,
                       postTaxCost,
                       true,
                       {}));
        } else {
            results.push_back(
                record(r,
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

Money AcquisitionManager::totalSpent() const {
    Money sum;

    for (const auto& rec : history_) {
        if (rec.approved) {
            sum += rec.cost;
        }
    }

    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    Money totalPreTax;
    Money totalTax;
    Money totalPostTax;

    os << "Order history (" << history_.size() << " orders)\n";

    for (const auto& rec : history_) {
        os << "  #" << std::setw(3) << std::left << rec.orderNo
           << " "
           << (rec.approved ? "APPROVED" : "REJECTED")
           << "  "
           << std::setw(6) << rec.resourceId
           << " x"
           << std::setw(3) << rec.quantity
           << " pre-tax "
           << std::setw(12) << std::right
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

        if (!rec.approved) {
            os << "\n        reason: " << rec.reason;
        }

        os << "\n";

        if (rec.approved) {
            totalPreTax += rec.preTaxCost;
            totalTax += rec.tax;
            totalPostTax += rec.cost;
        }
    }

    os << "Pre-tax total: " << totalPreTax << "\n";
    os << "Tax total: " << totalTax << "\n";
    os << "Post-tax total: " << totalPostTax << "\n";
    os << "Total spent: " << totalSpent() << "\n";
}

}  // namespace bookmgmt