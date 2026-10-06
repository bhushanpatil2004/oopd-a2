#include "bookmgmt/Journal.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

Journal::Journal(std::string id,
                 std::string title,
                 std::string issn,
                 int issuesPerYear,
                 std::string publisher,
                 int year,
                 Money annualPrice,
                 int subscriptionYears)
    : Resource(std::move(id),
               std::move(title),
               std::move(publisher),
               year,
               annualPrice),
      issn_(std::move(issn)),
      issuesPerYear_(issuesPerYear),
      subscriptionYears_(subscriptionYears) {
    if (subscriptionYears_ < 1) {
        throw std::invalid_argument(
            "journal subscription must be at least one year");
    }

    if (issuesPerYear_ < 1) {
        throw std::invalid_argument(
            "journal must have at least one issue per year");
    }
}

Money Journal::costFor(int copies) const {
    requirePositive(copies);

    // unitPrice() is the annual subscription price for one copy.
    const Money annualCost = unitPrice() * copies;
    return annualCost * subscriptionYears_;
}

void Journal::printDetails(std::ostream& os) const {
    os << "  issn: " << issn_ << "\n"
       << "  issues per year: " << issuesPerYear_ << "\n"
       << "  subscription years: " << subscriptionYears_ << "\n";
}

}  // namespace bookmgmt