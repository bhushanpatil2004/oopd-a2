#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, updating Catalog holdings and keeping an order history.

#include <iosfwd>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

struct PurchaseRequest {
    std::string resourceId;
    int quantity;  // copies for print, seats for electronic
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money preTaxCost;
    Money tax;
    Money cost;  // final cost after tax
    bool approved;
    std::string reason;  // why it was rejected; empty if approved
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget,
                   int printTaxPercent = 0,
                   int electronicTaxPercent = 0);

    void setTaxRates(int printTaxPercent, int electronicTaxPercent);

    int printTaxPercent() const { return printTaxPercent_; }
    int electronicTaxPercent() const { return electronicTaxPercent_; }

    // Price of a request without buying anything. Throws NotFoundError.
    Money quote(const std::string& id, int quantity) const;

    // True if the purchase would be approved; if not, `reason` explains why.
    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr) const;

    // Buys immediately. Throws NotFoundError, QuotaExceededError,
    // BudgetExceededError or std::invalid_argument. On success the budget
    // and holdings are updated and the record is added to history.
    const PurchaseRecord& purchase(const std::string& id, int quantity);

    // Processes requests in order; each is approved or rejected on its own
    // (never throws for a rejected request). Every outcome is recorded.
    // EXTENSION POINT: priority ordering, all-or-nothing batches, ...
    std::vector<PurchaseRecord> processBatch(const std::vector<PurchaseRequest>& reqs);

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalSpent() const;

    void printReport(std::ostream& os) const;

private:
    Money purchaseCost(const Resource* r, int quantity) const;
    Money taxFor(const Resource* r, Money preTaxCost) const;
    PurchaseRecord& record(const Resource* r, const std::string& id, int qty,
                       Money preTaxCost, Money tax, Money cost,
                       bool approved, std::string reason);

    Catalog& catalog_;
    Budget& budget_;
    std::vector<PurchaseRecord> history_;
    int nextOrderNo_ = 1;

    int printTaxPercent_;
    int electronicTaxPercent_;
};

}  // namespace bookmgmt
