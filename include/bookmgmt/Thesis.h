#pragma once

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

class Thesis : public Resource {
public:
    Thesis(std::string id,
           std::string title,
           std::string university,
           std::string degree,
           std::string supervisor,
           int year);

    ResourceCategory category() const override {
        return ResourceCategory::Thesis;
    }

    const std::string& university() const {
        return university_;
    }

    const std::string& degree() const {
        return degree_;
    }

    const std::string& supervisor() const {
        return supervisor_;
    }

    Money costFor(int quantity) const override;

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string university_;
    std::string degree_;
    std::string supervisor_;
};

/*
 * Design note:
 *
 * Thesis derives directly from Resource because it is a catalogue item
 * rather than a licensed electronic resource. A thesis is usually free
 * of cost, so costFor() returns zero while the object still follows the
 * common Resource interface.
 */

}  // namespace bookmgmt