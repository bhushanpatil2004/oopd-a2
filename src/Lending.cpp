#include "bookmgmt/Lending.h"

#include <stdexcept>

namespace bookmgmt {

LendingManager::LendingManager(Catalog& catalog)
    : catalog_(catalog) {}

void LendingManager::borrowCopy(
    const std::string& patron,
    const std::string& resourceId) {

    if (patron.empty()) {
        throw std::invalid_argument("patron cannot be empty");
    }

    Resource& resource = catalog_.get(resourceId);

    if (resource.isDigital()) {
        throw std::invalid_argument(
            "electronic resources use sessions");
    }

    auto& copies = borrowed_[resourceId];

    if (copies.count(patron) != 0) {
        throw std::invalid_argument(
            "patron already borrowed this resource");
    }

    if (catalog_.holdings(resourceId) <=
        static_cast<int>(copies.size())) {
        throw std::invalid_argument(
            "no available copies");
    }

    copies.insert(patron);
}

void LendingManager::returnCopy(
    const std::string& patron,
    const std::string& resourceId) {

    Resource& resource = catalog_.get(resourceId);

    if (resource.isDigital()) {
        throw std::invalid_argument(
            "electronic resources use sessions");
    }

    auto it = borrowed_.find(resourceId);

    if (it == borrowed_.end() ||
        it->second.erase(patron) == 0) {
        throw std::invalid_argument(
            "patron has not borrowed this resource");
    }
}

void LendingManager::openSession(
    const std::string& patron,
    const std::string& resourceId) {

    if (patron.empty()) {
        throw std::invalid_argument("patron cannot be empty");
    }

    Resource& resource = catalog_.get(resourceId);

    if (!resource.isDigital()) {
        throw std::invalid_argument(
            "print resources use borrowing");
    }

    auto& active = sessions_[resourceId];

    if (active.count(patron) != 0) {
        throw std::invalid_argument(
            "patron already has an open session");
    }

    const int seats = catalog_.holdings(resourceId);

    if (static_cast<int>(active.size()) >= seats) {
        throw std::invalid_argument(
            "no available seats");
    }

    active.insert(patron);
}

void LendingManager::closeSession(
    const std::string& patron,
    const std::string& resourceId) {

    Resource& resource = catalog_.get(resourceId);

    if (!resource.isDigital()) {
        throw std::invalid_argument(
            "print resources use borrowing");
    }

    auto it = sessions_.find(resourceId);

    if (it == sessions_.end() ||
        it->second.erase(patron) == 0) {
        throw std::invalid_argument(
            "patron has no open session");
    }
}

int LendingManager::borrowedCopies(
    const std::string& resourceId) const {

    auto it = borrowed_.find(resourceId);

    return it == borrowed_.end()
        ? 0
        : static_cast<int>(it->second.size());
}

int LendingManager::openSessions(
    const std::string& resourceId) const {

    auto it = sessions_.find(resourceId);

    return it == sessions_.end()
        ? 0
        : static_cast<int>(it->second.size());
}

}  // namespace bookmgmt
