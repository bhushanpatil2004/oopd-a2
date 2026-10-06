#include "bookmgmt/EBook.h"

#include <ostream>
#include <utility>

namespace bookmgmt {

EBook::EBook(std::string id,
             std::string title,
             std::vector<std::string> authors,
             std::string isbn,
             std::string publisher,
             int year,
             Money unitPrice,
             std::string accessUrl,
             LicenseModel license,
             Money platformFee,
             FileFormat format,
             bool drmProtected)
    : ElectronicResource(std::move(id),
                         std::move(title),
                         std::move(publisher),
                         year,
                         unitPrice,
                         std::move(accessUrl),
                         license,
                         platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(format),
      drmProtected_(drmProtected) {}

const char* fileFormatName(FileFormat format) {
    switch (format) {
        case FileFormat::PDF:
            return "PDF";
        case FileFormat::EPUB:
            return "EPUB";
        case FileFormat::HTML:
            return "HTML";
    }

    return "Unknown";
}
/*
 * Design note:
 *
 * Book and EBook both need bibliographic information such as title,
 * publisher, publication year, authors and ISBN. With the required
 * hierarchy, some of that information is therefore represented in
 * separate classes.
 *
 * A possible refactoring would be to extract the genuinely shared
 * bibliographic data and behavior into a separate component such as
 * BibliographicInfo, and compose it into both Book and EBook.
 * This avoids forcing EBook into Book's print-oriented inheritance
 * hierarchy while still reducing duplicated data handling.
 */

void EBook::printDetails(std::ostream& os) const {
    // ElectronicResource already prints URL, license and platform fee.
    ElectronicResource::printDetails(os);

    os << "  authors: ";
    for (std::size_t i = 0; i < authors_.size(); ++i) {
        if (i != 0) {
            os << ", ";
        }
        os << authors_[i];
    }

    os << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  format: " << fileFormatName(format_) << "\n"
       << "  drm protected: " << (drmProtected_ ? "yes" : "no") << "\n";
}

}  // namespace bookmgmt