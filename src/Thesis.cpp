#include "bookmgmt/Thesis.h"

#include <ostream>
#include <utility>

namespace bookmgmt {

Thesis::Thesis(std::string id,
               std::string title,
               std::string university,
               std::string degree,
               std::string supervisor,
               int year)
    : Resource(std::move(id),
               std::move(title),
               university,
               year,
               Money{}),
      university_(std::move(university)),
      degree_(std::move(degree)),
      supervisor_(std::move(supervisor)) {}

Money Thesis::costFor(int quantity) const {
    requirePositive(quantity);
    return Money{};
}

void Thesis::printDetails(std::ostream& os) const {
    os << "  university: " << university_ << "\n"
       << "  degree: " << degree_ << "\n"
       << "  supervisor: " << supervisor_ << "\n";
}

}  // namespace bookmgmt