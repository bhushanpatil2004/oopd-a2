#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, title limits, updating Catalog holdings and keeping an
// order history.

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

using Department = std::string;

struct PurchaseRequest {
    Department department;
    std::string resourceId;
    int quantity;  // copies for print, seats for electronic
};

struct PurchaseRecord {
    int orderNo;
    Department department;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money preTaxCost;
    Money tax;
    Money cost;  // final cost after tax
    bool approved;
    std::string reason;  // why it was rejected; empty if approved

    // Q8: true only for a separate cancellation record.
    bool cancellation = false;
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget,
                       int printTaxPercent = 0,
                       int electronicTaxPercent = 0);

    void setTaxRates(int printTaxPercent, int electronicTaxPercent);

    int printTaxPercent() const { return printTaxPercent_; }
    int electronicTaxPercent() const { return electronicTaxPercent_; }

    // Q9: add a separate budget for a department.
    void addDepartment(const Department& department, Budget& budget);

    // Q9: returns the budget assigned to a department.
    Budget& departmentBudget(const Department& department);
    const Budget& departmentBudget(const Department& department) const;

    // Price of a request without buying anything. Uses the default
    // department budget only for compatibility; pricing itself is
    // independent of the budget.
    Money quote(const std::string& id, int quantity) const;

    // Q9: price a request for a named department.
    Money quote(const Department& department,
                const std::string& id,
                int quantity) const;

    // True if the purchase would be approved; if not, `reason` explains why.
    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr) const;

    // Q9: department-aware purchase check.
    bool canPurchase(const Department& department,
                     const std::string& id,
                     int quantity,
                     std::string* reason = nullptr) const;

    // Buys immediately using the default department budget.
    const PurchaseRecord& purchase(const std::string& id, int quantity);

    // Q9: department-aware purchase.
    const PurchaseRecord& purchase(const Department& department,
                                   const std::string& id,
                                   int quantity);

    // Processes requests in order; each is approved or rejected on its own
    // (never throws for a rejected request). Every outcome is recorded.
    // Q11: when allOrNothing is true, the entire batch is committed
    // only if every request can be approved.
    std::vector<PurchaseRecord> processBatch(
        const std::vector<PurchaseRequest>& reqs,
        bool allOrNothing = false);

    // Q8: cancels an approved order, refunds its budget/quota usage,
    // reduces holdings and adds a separate cancellation record.
    const PurchaseRecord& cancel(int orderNo);

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalSpent() const;

    void printReport(std::ostream& os) const;

private:
    Money purchaseCost(const Resource* r, int quantity) const;
    Money taxFor(const Resource* r, Money preTaxCost) const;

    PurchaseRecord& record(const Resource* r,
                           const Department& department,
                           const std::string& id,
                           int qty,
                           Money preTaxCost,
                           Money tax,
                           Money cost,
                           bool approved,
                           std::string reason);

    // Q8: checks whether an approved purchase of this resource still exists
    // after excluding the order that is being cancelled.
    bool hasActivePurchase(const std::string& resourceId,
                           int excludedOrderNo) const;

    // Q9: finds the budget assigned to a department.
    Budget* findDepartmentBudget(const Department& department);
    const Budget* findDepartmentBudget(
        const Department& department) const;

    Catalog& catalog_;

    // Q9: the original budget is the default department budget.
    Budget& defaultBudget_;

    // Q9: additional department budgets.
    std::map<Department, Budget*> departmentBudgets_;

    std::vector<PurchaseRecord> history_;
    int nextOrderNo_ = 1;

    int printTaxPercent_;
    int electronicTaxPercent_;

    // Q8: order numbers of original approved purchases that were cancelled.
    std::set<int> cancelledOrders_;
};

}  // namespace bookmgmt