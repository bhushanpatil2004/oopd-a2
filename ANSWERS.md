# OOPD Assignment 2 — Answers

## Questions Attempted

I attempted all questions from Q1 to Q11.

---

## Q1 — Journal

Added a `Journal` resource to the existing resource hierarchy.

### Design

- `Journal` derives from `Resource`.
- It stores the number of issues published per year and the subscription duration.
- Journal cost is calculated using the unit price, number of copies, and subscription years.
- Invalid subscription years and issue counts are rejected.
- The default subscription duration is one year.

### Testing

Tests were added for:

- Journal category
- Journal pricing
- Subscription duration
- Issues per year
- Invalid subscription values
- Polymorphic behaviour
- Journal budget quotas

---

## Q2 — EBook

Added an `EBook` resource for electronic books.

### Design

- `EBook` derives from `ElectronicResource`.
- It stores authors, ISBN, file format, and DRM information.
- The existing electronic-resource pricing mechanism is reused.
- Supported file formats include PDF, EPUB, and HTML.

A separate bibliographic-information class could have been used with composition, but inheritance was kept here because an EBook is an electronic resource and needs its access and licensing behaviour.

### Testing

Tests cover:

- EBook category
- Digital-resource behaviour
- ISBN
- Authors
- File format
- DRM
- Pricing
- Polymorphism
- EBook quotas

---

## Q3 — AudioBook and Thesis

Added `AudioBook` and `Thesis` resources.

### Design

- `AudioBook` derives from `ElectronicResource` because it requires electronic access and licensing behaviour.
- `Thesis` derives directly from `Resource` because it is a catalogue item and is normally free rather than being an electronically licensed resource.

### AudioBook

Stores:

- Narrator
- Duration in minutes
- Electronic access information
- Licensing information

### Thesis

Stores:

- University
- Degree
- Supervisor

Thesis cost is zero because theses are normally free in this assignment model.

### Testing

Tests were added for:

- AudioBook category and properties
- AudioBook pricing
- Thesis category and properties
- Thesis free pricing
- Polymorphic behaviour
- Separate quotas

---

## Q4 — Hardcover Pricing

Hardcover books cost 20% more than their listed unit price.

### Design

The pricing rule is implemented in `Book::costFor()`.

Paperback books use their normal listed price, while hardcover books use:

```text
listed price × 1.20
```

The calculation uses integer minor currency units to avoid floating-point money calculations.

### Testing

Tests verify:

- Paperback pricing
- Hardcover pricing
- Multiple copy quantities
- Correct 20% hardcover increase

---

## Q5 — Bulk Discounts

Two bulk-pricing rules were implemented.

### Print Resources

Print resources receive a 10% discount when 10 or more copies are purchased at once.

In the implementation, the following categories are treated as print-resource categories for this rule:

- `Book`
- `Journal`
- `Thesis`

The discount is implemented in `AcquisitionManager` because it depends on the quantity of a purchase rather than the resource's normal unit pricing.

### Electronic Resources

For electronic resources, the first 50 seats are charged at the normal seat price.

Every seat beyond the 50th costs half price.

This rule is implemented in `ElectronicResource::costFor()`.

### Testing

Tests cover:

- Print purchases below 10 copies
- Print purchases at 10 copies
- Electronic purchases at 50 seats
- Electronic purchases above 50 seats
- Electronic purchases with multiple extra seats

---

## Q6 — Taxes

Configurable tax rates were added for print and electronic resources.

### Design

The resource's normal cost remains the **pre-tax cost**.

The acquisition process then performs the following sequence:

1. Calculate the resource cost.
2. Apply the Q5 bulk-discount rules.
3. Calculate tax using the configured category tax rate.
4. Calculate the post-tax cost.
5. Check the budget and category quotas using the post-tax cost.
6. Commit the post-tax cost when the purchase is approved.

Two configurable rates are supported:

- Print tax rate
- Electronic tax rate

Tax rates are represented as whole percentages from 0 to 100.

### Reporting

The acquisition report shows:

- Pre-tax cost
- Tax
- Post-tax cost

It also reports:

- Pre-tax total
- Tax total
- Post-tax total
- Total spent

### Testing

Tests verify:

- Print tax calculation
- Electronic tax calculation
- Configurable tax rates
- Invalid tax-rate rejection
- Post-tax budget spending
- Post-tax category quota checking

A specific test verifies that a purchase can be rejected when its pre-tax cost is within a quota but its post-tax cost exceeds the quota.

---

## Q7 — Different-Title Limits

A limit was added on the number of different titles that may be purchased in each category.

### Design

The title limit is maintained by `Budget`.

Each category can have an optional maximum number of different titles.

For example:

```text
Book title limit = 2
```

The resource ID is used as the title identity because every catalogue resource has a unique ID.

The title limit is separate from the existing unit and spending quotas.

A purchase of an already-used title does not consume another title slot.

For example, if the Book title limit is 2 and `B001` has already been purchased, purchasing more copies of `B001` does not increase the number of different titles used.

### Testing

Tests verify:

- Setting a title limit
- Reading a title limit
- Counting titles used
- Purchasing different titles
- Purchasing the same title multiple times
- Rejecting a new title after the limit is reached
- Zero title limits
- Invalid negative title limits
- Batch processing with title limits
- Preservation of title usage when a purchase is rejected

---

## Q8 — Cancellation

Approved purchases can be cancelled using their order number.

### Design Decision

The original approved purchase record is kept in the order history, and a separate cancellation record is added.

The cancellation record is marked with:

```cpp
cancellation = true;
```

When an order is cancelled:

- The post-tax purchase cost is refunded to the overall budget.
- The category's unit usage is reduced.
- The category's spending usage is reduced.
- Catalogue holdings are reduced by the cancelled quantity.
- The title limit is released only when no other active approved order still uses the same resource.
- A cancelled order cannot be cancelled again.
- Rejected orders cannot be cancelled.
- Cancellation records cannot be cancelled.
- An unknown order number raises `NotFoundError`.

The cancellation implementation was added to `AcquisitionManager`, while `Budget` provides `refund()` and `releaseTitle()` to restore the relevant budget and title-limit state.

### History and Reporting

The original purchase remains in `history()`.

A separate cancellation record is appended to the history so that the complete sequence of actions is preserved.

Cancelled purchases are excluded from active spending totals and budget reports.

Cancellation records have zero cost because the original purchase amount has already been refunded.

### Testing

Tests verify:

1. Cancellation of an approved order.
2. Refund of overall budget usage.
3. Refund of category unit usage.
4. Refund of category spending usage.
5. Reduction of catalogue holdings.
6. Preservation of a title slot when another active purchase uses the same title.
7. Release of the title slot after the final active purchase of that title is cancelled.
8. Purchasing a new title after the slot is released.
9. Rejection of cancellation for rejected orders.
10. Rejection of repeated cancellation.
11. Rejection of cancellation records.
12. `NotFoundError` for an unknown order number.
13. Preservation of the original purchase record.
14. Creation of a separate cancellation record.

---

## Q9 — Department Budgets

Each department has its own budget and quotas.

### Design

Each department has its own `Budget` object containing:

- Total budget
- Category quotas
- Title limits
- Spending usage
- Category unit usage
- Purchased-title usage

`Department` is represented as:

```cpp
using Department = std::string;
```

The purchase request contains the department that should be charged:

```cpp
struct PurchaseRequest {
    Department department;
    std::string resourceId;
    int quantity;
};
```

Purchase records also store the department so that the acquisition history records which department paid for each purchase.

The existing single-budget constructor is preserved. Its budget is treated as the `"Default"` department so that earlier functionality remains compatible.

### Department Operations

The acquisition manager supports:

- Adding departments
- Accessing department budgets
- Department-specific quotations
- Department-specific purchase checks
- Department-specific purchases
- Department-specific batch processing

Each department's quotas are checked independently.

For example, a Book title limit reached by the Physics department does not affect the Computer Science department.

### Cancellation

Cancellation uses the department stored in the original purchase record.

Therefore, when a department purchase is cancelled, the refund is applied to the correct department's budget.

### Testing

Tests verify:

- Separate budgets for different departments
- Department-specific spending
- Department-specific unit quotas
- Department-specific title limits
- Department-aware purchase requests
- Department-aware purchase history
- Rejection of purchases when a department quota is exceeded
- Unknown department handling
- Correct department refund during cancellation

---

## Q10 — Year-End Budget Rollover

Create the next year's budget from the current year's budget by carrying forward a configurable percentage of the unspent amount.

### Design Decision

I implemented:

```cpp
Budget Budget::rollover(int percentage) const;
```

The rollover percentage must be between 0 and 100.

The new budget is calculated from the current year's **unspent amount**, not from the original total budget.

For example:

```text
Current budget  = ₹10,000
Current spending = ₹6,000
Unspent amount  = ₹4,000
Rollover        = 50%

Next year's budget = 50% × ₹4,000
                   = ₹2,000
```

### What Is Carried Forward

The following configuration is carried forward to the next year's budget:

- Per-category unit quotas
- Per-category spending quotas
- Q7 different-title limits

### What Is Reset

The following values are intentionally reset for the new financial year:

- Amount spent
- Category usage
- Purchased-title history

Therefore, a newly created rollover budget starts with zero spending and zero purchased titles.

For example, if the previous year used 6 Book units and purchased 1 Book title, the next year's budget starts with:

```text
Book units used  = 0
Book titles used = 0
```

This is intentional because the previous year's purchases should not count as purchases in the new financial year.

### Original Budget

The original budget is not modified by the rollover operation.

The method creates and returns a separate `Budget` object for the next year.

### Validation

The implementation rejects rollover percentages below 0 or above 100.

### Testing

Tests verify:

- 50% rollover of the unspent amount
- 0% rollover
- 100% rollover
- Carrying forward category quotas
- Carrying forward title limits
- Resetting category usage
- Resetting purchased-title history
- Preserving the original budget
- Rejecting invalid percentages

---

## Q11 — All-or-Nothing Batch Processing

An option was added to make `processBatch()` operate in an all-or-nothing mode.

### Requirement

The existing batch-processing behaviour is preserved, while an optional flag allows a complete batch to be accepted or rejected as one unit.

The interface is:

```cpp
std::vector<PurchaseRecord> processBatch(
    const std::vector<PurchaseRequest>& reqs,
    bool allOrNothing = false);
```

### Normal Mode

When:

```cpp
allOrNothing == false
```

the existing sequential behaviour is preserved.

Each request is processed independently. Therefore, one request may be approved even if a later request is rejected.

### All-or-Nothing Mode

When:

```cpp
allOrNothing == true
```

the complete batch is validated before modifying the real system state.

Temporary copies of the department budgets are used during validation.

These temporary budgets simulate:

- Overall budget spending
- Category unit usage
- Category spending usage
- Different-title limits
- Purchased-title tracking

Requests are validated sequentially against the temporary budgets so that an earlier request can affect the validation of a later request in the same batch.

### Successful Batch

If every request passes validation:

1. The real department budgets are updated.
2. Category usage is updated.
3. Title usage is updated.
4. Catalogue holdings are increased.
5. Purchase records are added to history.

Thus, the entire batch is committed only after the complete batch has been validated successfully.

### Failed Batch

If any request fails validation:

- No budget is changed.
- No category quota is changed.
- No title slot is consumed.
- No catalogue holdings are changed.
- No purchase records are added to history.

The all-or-nothing operation returns an empty result because no request from the failed batch was actually processed as a purchase.

### Testing

Tests verify:

1. A valid all-or-nothing batch approves all requests.
2. All requests in a successful batch affect the budget.
3. All requests in a successful batch update catalogue holdings.
4. Different titles consume the appropriate title slots.
5. A batch containing an invalid resource is rejected completely.
6. A valid request before the invalid request is not purchased.
7. Budget spending remains unchanged after a failed batch.
8. Category unit usage remains unchanged after a failed batch.
9. Category spending usage remains unchanged after a failed batch.
10. Title usage remains unchanged after a failed batch.
11. Catalogue holdings remain unchanged after a failed batch.
12. Order history remains unchanged after a failed batch.
13. Existing normal batch-processing behaviour remains unchanged.

The final test suite passes:

```text
344/344 checks passed
```

---

## Final Verification

All questions from Q1 to Q11 were implemented with corresponding tests and demo examples.

Final automated test result:

```text
344/344 checks passed
```

The demo was also executed successfully and demonstrates the functionality added through Q1–Q11.


Q13 — Catalog Searches

Implementation:
- Added search by author using case-insensitive partial matching for Book and EBook authors.
- Added search by ISBN/ISSN using case-insensitive exact matching.
- ISBN search supports Book and EBook resources, while ISSN search supports Journal resources.
- Added inclusive publication-year range search across catalog resources.
- Invalid year ranges where fromYear > toYear throw std::invalid_argument.

Design decision:
The search operations are implemented in Catalog because Catalog owns the resources and already provides the general search infrastructure.