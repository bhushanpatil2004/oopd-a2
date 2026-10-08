# OOPD Assignment 2 — Answers

## Questions Attempted

I attempted all questions from Q1 to Q7.

---

## Q1 — Journal

Added a `Journal` resource to the existing resource hierarchy.

### Design

- `Journal` derives from `Resource`.
- It stores the number of issues published per year and the subscription duration.
- Journal cost is calculated using the unit price, number of copies, and subscription years.
- Invalid subscription years and issue counts are rejected.

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

A separate bibliographic-information class could have been used with composition, but inheritance was kept here because an EBook is an electronic resource and needs its access/licensing behaviour.

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

`listed price × 1.20`

The calculation uses integer minor currency units to avoid floating-point money calculations.

### Testing

Tests verify both paperback and hardcover pricing for multiple copy quantities.

---

## Q5 — Bulk Discounts

Two bulk-pricing rules were implemented.

### Print resources

Print resources receive a 10% discount when 10 or more copies are purchased at once.

In the implementation, `Book`, `Journal`, and `Thesis` are treated as print-resource categories for this rule.

The discount is implemented in `AcquisitionManager` because it depends on the quantity of a purchase rather than the resource's normal unit pricing.

### Electronic resources

For electronic resources, the first 50 seats are charged at the normal seat price. Every seat beyond the 50th costs half price.

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

## Q8 — Cancellation

### Design decision

An approved purchase can be cancelled using its order number.

The original approved purchase record is kept unchanged in the order
history, and a separate cancellation record is added. The cancellation
record is marked with `cancellation = true`.

When an order is cancelled:

- The post-tax purchase cost is refunded to the overall budget.
- The category's unit usage is reduced.
- The category's spending usage is reduced.
- Catalog holdings are reduced by the cancelled quantity.
- The title limit is released only when no other active approved order
  still uses the same resource.
- A cancelled order cannot be cancelled again.
- Rejected orders and cancellation records cannot be cancelled.
- An unknown order number raises `NotFoundError`.

The cancellation implementation was added to `AcquisitionManager`, while
`Budget` provides `refund()` and `releaseTitle()` to restore the relevant
budget and title-limit state.

### History and reporting

The original purchase remains in `history()`. A separate cancellation
record is appended to the history so that the complete sequence of
actions is preserved.

Cancelled purchases are excluded from active spending totals and budget
reports. Cancellation records have zero cost because the original
purchase amount has already been refunded.

### Testing

Tests verify:

1. Cancellation of an approved order.
2. Refund of overall budget usage.
3. Refund of category unit and spending quotas.
4. Reduction of catalog holdings.
5. Preservation of a title slot when another active purchase uses the
   same title.
6. Release of the title slot after the final active purchase of that
   title is cancelled.
7. Purchasing a new title after the slot is released.
8. Rejection of cancellation for rejected orders.
9. Rejection of repeated cancellation.
10. Rejection of cancellation records.
11. `NotFoundError` for an unknown order number.
12. Preservation of the original purchase record and creation of a
    separate cancellation record.

## Q9 — Department Budgets

### Design
Each department has its own `Budget` object containing its total budget,
category quotas, title limits, spending, and title usage.

`Department` is represented as:

```cpp
using Department = std::string;


## Q10 — Year-end Budget Rollover

### Requirement
Create the next year's budget from the current year's budget by carrying forward a configurable percentage of the unspent amount.

### Design Decision
I implemented a `Budget::rollover(int percentage)` method.

The rollover percentage must be between 0 and 100. The new budget is calculated from the current year's **unspent amount**, not from the original total budget.

For example:

- Current budget = ₹10,000
- Current spending = ₹6,000
- Unspent amount = ₹4,000
- Rollover percentage = 50%
- Next year's budget = 50% × ₹4,000 = ₹2,000

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

- Book units used = 0
- Book titles used = 0

This is intentional because the previous year's purchases should not count as purchases in the new financial year.

### Original Budget

The original budget is not modified by the rollover operation. The method creates and returns a separate `Budget` object for the next year.

### Validation

The implementation rejects rollover percentages below 0 or above 100.

The tests verify:

- 50% rollover of the unspent amount
- 0% rollover
- 100% rollover
- Carrying forward category quotas
- Carrying forward title limits
- Resetting category usage
- Resetting purchased-title history
- Preserving the original budget
- Rejecting invalid percentages