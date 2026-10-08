#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "bookmgmt/Catalog.h"

namespace bookmgmt {

class LendingManager {
public:
    explicit LendingManager(Catalog& catalog);

    void borrowCopy(const std::string& patron,
                    const std::string& resourceId);

    void returnCopy(const std::string& patron,
                    const std::string& resourceId);

    void openSession(const std::string& patron,
                     const std::string& resourceId);

    void closeSession(const std::string& patron,
                      const std::string& resourceId);

    int borrowedCopies(const std::string& resourceId) const;
    int openSessions(const std::string& resourceId) const;

private:
    Catalog& catalog_;

    std::unordered_map<
        std::string,
        std::unordered_set<std::string>>
        borrowed_;

    std::unordered_map<
        std::string,
        std::unordered_set<std::string>>
        sessions_;
};

}  // namespace bookmgmt
