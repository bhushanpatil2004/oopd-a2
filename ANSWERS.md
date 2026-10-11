# OOPD Assignment 2 — Answers

## Questions Attempted

I attempted all questions from Q1 to Q16. The following sections describe the implementation, design decisions, and testing performed for each question.

---

## Q1 — Journal

Added a `Journal` resource to the existing resource hierarchy.

### Design

- `Journal` derives from `Resource`.
- It stores the ISSN, number of issues published per year, and subscription duration.
- The default subscription duration is one year.
- Subscription duration must be at least one year.
- Invalid subscription durations and issue counts are rejected.
- The annual subscription price is used to calculate the cost of purchasing journal copies over the selected subscription period.
- `Journal` has its own `ResourceCategory` value so that journal purchases can have separate quotas.
- `printDetails()` displays the journal-specific fields.

The cost is calculated as:

```text
costFor(copies) = unitPrice × copies × subscriptionYears
```

### Testing

Tests cover:

- Journal category and properties
- Journal pricing
- Subscription duration
- Number of issues per year
- Invalid subscription values
- Polymorphic behaviour
- Journal budget quotas

---

## Q2 — EBook

Added an `EBook` resource for electronic books.

### Design

- `EBook` derives from `ElectronicResource`.
- It stores authors, ISBN, file format, and DRM information.
- Supported file formats include PDF, EPUB, and HTML.
- The existing electronic-resource pricing mechanism is reused.
- `EBook` has its own `ResourceCategory` value so that e-books can have quotas separate from other electronic resources.
- `printDetails()` calls the parent implementation before displaying the additional e-book information.

### Design Question: Avoiding Duplication

Some bibliographic information and related operations are common to both `Book` and `EBook`. These can be duplicated when the two classes maintain separate implementations for authors, ISBN, and other shared metadata.

A possible improvement would be to extract the common bibliographic information into a reusable class and use composition. This would reduce duplication while allowing `Book` and `EBook` to retain their distinct physical and electronic access behaviours.

Inheritance was retained in this implementation because an `EBook` is an electronic resource and needs its access and licensing behaviour.

### Testing

Tests cover:

- EBook category
- Digital-resource behaviour
- ISBN and authors
- File format and DRM
- Pricing
- Polymorphism
- EBook quotas

---

## Q3 — AudioBook and Thesis

Added `AudioBook` and `Thesis` resources to the class hierarchy.

### AudioBook

`AudioBook` derives from `ElectronicResource` because it requires electronic access and licensing behaviour.

It stores:

- Narrator
- Duration in minutes
- Electronic access information
- Licensing information

The existing electronic-resource pricing mechanism is reused.

### Thesis

`Thesis` derives directly from `Resource` because it represents a catalogue item rather than a separately licensed electronic platform resource.

It stores:

- University
- Degree
- Supervisor

Thesis cost is zero in this assignment model because theses are treated as normally free resources.

### Testing

Tests cover:

- AudioBook category and properties
- AudioBook pricing
- Thesis category and properties
- Thesis free pricing
- Polymorphic behaviour
- Separate category quotas

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
- Multiple-copy quantities
- The correct 20% hardcover increase

---

## Q5 — Bulk Discounts

Two bulk-pricing rules were implemented.

### Print Resources

Print resources receive a 10% discount when 10 or more copies are purchased at once.

The following categories are treated as print-resource categories for this rule:

- `Book`
- `Journal`
- `Thesis`

The discount is implemented in `AcquisitionManager` because it depends on the quantity of a purchase rather than only on the resource's normal unit pricing.

### Electronic Resources

For electronic resources, the first 50 seats are charged at the normal seat price. Every seat beyond the 50th costs half price.

This rule is implemented in `ElectronicResource::costFor()`.

### Testing

Tests cover:

- Print purchases below 10 copies
- Print purchases at 10 copies
- Electronic purchases at 50 seats
- Electronic purchases above 50 seats
- Electronic purchases containing multiple seats beyond the 50th

---

## Q6 — Taxes

Configurable tax rates were added for print and electronic resources.

### Design

The resource's normal cost is treated as the pre-tax cost.

The acquisition process follows this sequence:

1. Calculate the resource cost.
2. Apply the applicable bulk-discount rules.
3. Calculate tax using the configured tax rate.
4. Calculate the post-tax cost.
5. Check the budget and category quotas using the post-tax cost.
6. Commit the post-tax cost when the purchase is approved.

Two configurable tax rates are supported:

- Print tax rate
- Electronic tax rate

Tax rates are represented as whole percentages from 0 to 100.

### Reporting

The acquisition report shows:

- Pre-tax cost
- Tax
- Post-tax cost
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

Added a limit on the number of different titles that may be purchased in each category.

### Design

The title limit is maintained by `Budget`.

Each category can have an optional maximum number of different titles. The resource ID is used as the title identity because every catalogue resource has a unique ID.

The title limit is separate from the existing unit and spending quotas.

A purchase of an already-used title does not consume another title slot.

For example, if the Book title limit is two and `B001` has already been purchased, purchasing more copies of `B001` does not increase the number of different titles used.

### Testing

Tests cover:

- Setting and reading a title limit
- Counting titles used
- Purchasing different titles
- Purchasing the same title multiple times
- Rejecting a new title after the limit is reached
- Zero title limits
- Invalid negative title limits
- Batch processing with title limits
- Preserving title usage when a purchase is rejected

---

## Q8 — Cancellation

Approved purchases can be cancelled using their order number.

### Design

The original approved purchase record is kept in the order history, and a separate cancellation record is added.

The cancellation record is marked with:

```cpp
cancellation = true;
```

When an order is cancelled:

- The post-tax purchase cost is refunded to the overall budget.
- Category unit usage is reduced.
- Category spending usage is reduced.
- Catalogue holdings are reduced by the cancelled quantity.
- The title limit is released only when no other active approved order uses the same resource.
- A cancelled order cannot be cancelled again.
- Rejected orders cannot be cancelled.
- Cancellation records cannot be cancelled.
- An unknown order number raises `NotFoundError`.

The cancellation implementation is handled by `AcquisitionManager`, while `Budget` provides the refund and title-release operations.

### History and Reporting

The original purchase remains in `history()`.

A separate cancellation record is appended so that the complete sequence of actions is preserved.

Cancelled purchases are excluded from active spending totals and budget reports. Cancellation records have zero cost because the original purchase amount has already been refunded.

### Testing

Tests cover:

1. Cancellation of an approved order.
2. Refund of overall budget usage.
3. Refund of category unit usage.
4. Refund of category spending usage.
5. Reduction of catalogue holdings.
6. Preservation of a title slot when another active purchase uses the same title.
7. Release of the title slot after the final active purchase is cancelled.
8. Purchasing a new title after a slot is released.
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

A purchase request contains the department that should be charged:

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

Each department's quotas are checked independently. For example, a Book title limit reached by the Physics department does not affect the Computer Science department.

### Cancellation

Cancellation uses the department stored in the original purchase record. Therefore, the refund is applied to the correct department's budget.

### Testing

Tests cover:

- Separate budgets for different departments
- Department-specific spending
- Department-specific unit quotas
- Department-specific title limits
- Department-aware purchase requests
- Department-aware purchase history
- Rejection when a department quota is exceeded
- Unknown department handling
- Correct department refund during cancellation

---

## Q10 — Year-End Budget Rollover

Implemented year-end budget rollover to create the next year's budget by carrying forward a configurable percentage of the unspent amount.

### Design

The implemented interface is:

```cpp
Budget Budget::rollover(int percentage) const;
```

The rollover percentage must be between 0 and 100.

The new budget is calculated from the current year's unspent amount, not from the original total budget.

For example:

```text
Current budget   = ₹10,000
Current spending = ₹6,000
Unspent amount   = ₹4,000
Rollover         = 50%

Next year's budget = 50% × ₹4,000
                   = ₹2,000
```

### What Is Carried Forward

The following configuration is carried forward:

- Per-category unit quotas
- Per-category spending quotas
- Different-title limits

### What Is Reset

The following values are reset for the new financial year:

- Amount spent
- Category usage
- Purchased-title history

Therefore, the new rollover budget starts with zero spending and zero purchased titles.

The original budget remains unchanged. The rollover method returns a separate `Budget` object for the next year.

### Validation

Rollover percentages below 0 or above 100 are rejected.

### Testing

Tests cover:

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

Added an option to make `processBatch()` operate in all-or-nothing mode.

### Interface

The implemented interface is:

```cpp
std::vector<PurchaseRecord> processBatch(
    const std::vector<PurchaseRequest>& reqs,
    bool allOrNothing = false);
```

### Normal Mode

When `allOrNothing` is `false`, the existing sequential behaviour is preserved. Each request is processed independently, so one request may be approved even if a later request is rejected.

### All-or-Nothing Mode

When `allOrNothing` is `true`, the complete batch is validated before modifying the real system state.

Temporary copies of the department budgets are used during validation. These temporary budgets simulate:

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

### Failed Batch

If any request fails validation:

- No budget is changed.
- No category quota is changed.
- No title slot is consumed.
- No catalogue holdings are changed.
- No purchase records are added to history.

The all-or-nothing operation returns an empty result because no purchase from the failed batch was committed.

### Testing

Tests cover:

- A valid all-or-nothing batch
- Budget and quota updates after a successful batch
- Catalogue holding updates
- Different-title usage
- Rejection of a batch containing an invalid resource
- No purchase of earlier valid requests when a later request fails
- Unchanged budget spending after failure
- Unchanged category unit and spending usage after failure
- Unchanged title usage after failure
- Unchanged catalogue holdings after failure
- Unchanged order history after failure
- Preservation of normal batch-processing behaviour

---

## Q12 — Vendors

Added vendor-aware pricing so that the same catalogue resource can be offered by multiple vendors at different prices.

### Design

- Vendor offers store the vendor name and price for a resource.
- Multiple vendor offers can be registered for the same resource.
- The acquisition manager selects the cheapest registered offer.
- Vendor-aware pricing is used when calculating quotations and purchases.
- Approved purchase records store the vendor used.
- The acquisition history and reports display vendor information.
- Resources without registered vendor offers continue to use their normal pricing.

The selected vendor and its price are associated with the resource being purchased, allowing the acquisition system to choose the most economical available offer.

### Demo

The demo demonstrates:

- Registering multiple vendors for a resource
- Selecting the cheapest vendor
- Updating the selected vendor when a cheaper offer is registered
- Selecting a cheapest vendor for another resource
- Displaying the selected vendor in quotations and purchase records
- Showing vendor information in batch purchases and reports

### Testing

The vendor-related tests and demo exercise the selection of the cheapest available offer and the recording of vendor information in purchase history.

---

## Q13 — Catalogue Searches

Added catalogue search operations by author, ISBN/ISSN, and publication-year range.

### Implementation

- Author search supports case-insensitive partial matching for `Book` and `EBook` authors.
- ISBN search supports `Book` and `EBook` resources.
- ISSN search supports `Journal` resources.
- Publication-year search uses an inclusive year range across catalogue resources.
- An invalid range where `fromYear > toYear` throws `std::invalid_argument`.

### Design Decision

The search operations are implemented in `Catalog` because it owns the resources and provides the catalogue search infrastructure.

The assignment's Q13 requirements are limited to searching. Catalogue save/load is not included in the newer question list.

### Testing and Demo

The tests and demo cover catalogue searches by author, ISBN/ISSN, and publication-year range.

---

## Q14 — Lending

The lending functionality supports borrowing and returning print copies and opening and closing sessions on electronic resources.

### Design

For print resources:

- Patrons can borrow available copies.
- Borrowing is limited by the number of copies held.
- Borrowed copies can be returned.

For electronic resources:

- Patrons can open and close electronic-resource sessions.
- Active sessions are limited by the number of licensed seats.

The lending operations enforce the available-copy and licensed-seat limits.

### Demo

The demo demonstrates print-copy borrowing and returning, as well as opening and closing electronic-resource sessions.

---

## Q15 — Money Currency

Added a currency code to the `Money` class.

### Implementation

The existing `Money::of()` and `Money::fromMinor()` interfaces are preserved through default currency values.

Each `Money` object stores:

- The amount in minor currency units
- A currency code, such as `INR` or `USD`

The default currency is `INR`, allowing existing calls to continue working without modification.

### Currency Validation

Addition, subtraction, and comparison operations check that both `Money` objects use the same currency.

If the currencies differ, the operation throws `std::invalid_argument`.

Multiplication by an integer operates on a single `Money` object and does not require a currency comparison.

### Testing

Tests cover:

- Storage of currency codes
- Existing `Money::of()` calls
- `Money::fromMinor()` with a currency code
- Addition and subtraction with matching currencies
- Addition with different currencies
- Subtraction with different currencies
- Comparison of different currencies
- Rejection of empty currency codes

### Demo

The demo shows INR and USD amounts, performs a same-currency addition, and demonstrates rejection of an operation involving different currencies.

### Design Decision

The currency is stored directly inside `Money` so that currency consistency is enforced at the value-operation level. This prevents accidental addition, subtraction, or comparison of monetary values belonging to different currencies while retaining the fixed-point representation.

---

## Q16 — Code Cleanliness

Replaced the test macros `CHECK` and `CHECK_THROWS` with standard C++ `assert` and a templated `assertThrows` helper.

### Implementation

- Ordinary test checks use `assert(condition)`.
- Exception checks use a templated `assertThrows` helper that verifies whether the required exception type is thrown.
- The original test expressions and their intended meanings are preserved.
- The manual test counters were removed because standard `assert` is now used for validating test conditions.

### Design Decision

A templated `assertThrows` helper was used instead of a macro so that exception checking remains type-safe C++ code while preserving the purpose of the original `CHECK_THROWS` tests.

Q16 is a test-code cleanup task and does not introduce a new library feature, so no additional functionality was required in `examples/demo.cpp`.

---

## Final Verification

The project was configured and built using CMake. The test executable and demo were run successfully.

### Build and Test Commands

```bash
cmake -S . -B build
cmake --build build -j2
./build/bookmgmt_tests
./build/demo
ctest --test-dir build --output-on-failure
```

### Verification Results

- The project compiled successfully.
- The test executable reported `All tests passed`.
- The demo executed and displayed the implemented functionality.
- CTest reported `1/1` test passed and `100% tests passed, 0 tests failed`.

The successful test run verifies the test suite included in the submitted project. It does not, by itself, establish that every possible edge case has been tested.

### Submission Contents

The submission should contain the complete project directory, including:

- Source and header files
- `CMakeLists.txt`
- Tests
- Demo source code
- `README.md`
- This `ANSWERS.md`
- The `.git` directory, as required by the assignment
