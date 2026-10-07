# OOPD Assignment 2 — Answers

## Questions Attempted

I attempted all questions from Q1 to Q6.

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

## Overall Design Decisions

The implementation keeps resource-specific pricing inside the resource classes where appropriate, while purchase-dependent rules are handled by `AcquisitionManager`.

Money calculations use integer minor currency units rather than floating-point arithmetic.

Existing functionality from earlier questions was preserved while adding each new requirement.

All existing tests continue to pass after the Q6 changes.
