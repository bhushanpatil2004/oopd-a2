#pragma once

#include <string>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

class AudioBook : public ElectronicResource {
public:
    AudioBook(std::string id,
              std::string title,
              std::string narrator,
              int durationMinutes,
              std::string publisher,
              int year,
              Money pricePerSeat,
              std::string accessUrl,
              LicenseModel license = LicenseModel::AnnualSubscription,
              Money platformFee = Money{});

    ResourceCategory category() const override {
        return ResourceCategory::AudioBook;
    }

    const std::string& narrator() const {
        return narrator_;
    }

    int durationMinutes() const {
        return durationMinutes_;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string narrator_;
    int durationMinutes_;
};

/*
 * Design note:
 *
 * AudioBook derives from ElectronicResource because an audiobook is a
 * digital resource accessed through an electronic platform. It can
 * therefore reuse the existing access URL, licensing and seat-pricing
 * behavior of ElectronicResource.
 */

}  // namespace bookmgmt