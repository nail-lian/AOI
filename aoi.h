#ifndef AOI_H
#define AOI_H

#include <cstdio>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cassert>

const float kDefaultVisibleRange = 64;

class AOI {
 public:
  struct Unit;

  typedef std::function<void(int, int)> Callback;
  typedef std::unordered_set<Unit*> UnitSet;
  struct Unit {
    Unit(int id_, float x_, float y_, Callback enter_callback_,
         Callback leave_callback_)
        : id(id_),
          x(x_),
          y(y_),
          enter_callback(enter_callback_),
          leave_callback(leave_callback_) {}

    Unit(int id_, float x_, float y_)
        : id(id_),
          x(x_),
          y(y_),
          enter_callback(nullptr),
          leave_callback(nullptr) {}

    ~Unit(){};

    void Subscribe(Unit* const other) { subscribe_set.insert(other); }
    void UnSubscribe(Unit* const other) { subscribe_set.erase(other); }

    int id;
    float x;
    float y;
    Callback enter_callback;
    Callback leave_callback;
    UnitSet subscribe_set;
  };

 public:
  AOI(const float width, const float height,
      const float visible_range = kDefaultVisibleRange)
      : width_(width), height_(height), visible_range_(visible_range) {}

  virtual ~AOI() {
    for (const auto& pair : unit_map_) {
      Unit* unit = pair.second;
      delete unit;
    }
  };

  AOI(const AOI&) = delete;
  AOI(AOI&&) = delete;
  AOI& operator=(const AOI&) = delete;
  AOI& operator=(AOI&&) = delete;

  // Update when unit moved
  // id is a custom integer
  virtual void UpdateUnit(int id, float x, float y) {
    (void)id;
    assert(x <= width_);
    assert(y <= width_);
  };

  // Add unit to AOI
  // id is a custom integer
  virtual Unit* AddUnit(int id, float x, float y, Callback enter_callback,
                        Callback leave_callback) {
    assert(x <= width_ && y <= height_);
    assert(unit_map_.find(id) == unit_map_.end());

    Unit* unit = new Unit(id, x, y, enter_callback, leave_callback);
    unit_map_.insert(std::pair(unit->id, unit));
    return unit;
  };

  // Remove unit from AOI
  // id is a custom integer
  virtual void RemoveUnit(int id) {
    Unit* unit = unit_map_[id];
    unit_map_.erase(id);
    delete unit;
  };

  // Find the ids of units in range near the given id, and exclude id itself
  std::unordered_set<int> FindNearbyUnit(int id, const float range) const {
    Unit* unit = get_unit(id);
    UnitSet unit_set = FindNearbyUnit(unit, range);
    std::unordered_set<int> id_set;
    for (const auto& unit : unit_set) {
      id_set.insert(unit->id);
    }
    return id_set;
  };

  const float& get_width() const { return width_; }
  const float& get_height() const { return height_; }

 protected:
  virtual UnitSet FindNearbyUnit(Unit* unit, const float range) const = 0;

  Unit* get_unit(int id) const { return unit_map_[id]; }

  UnitSet Intersection(const UnitSet& set, const UnitSet& other) const {
    UnitSet res;
    for (const auto& unit : set) {
      if (other.find(unit) != other.end()) {
        res.insert(unit);
      }
    }
    return res;
  }

  UnitSet Difference(const UnitSet& set, const UnitSet& other) const {
    UnitSet res;
    for (const auto& unit : set) {
      if (other.find(unit) == other.end()) {
        res.insert(unit);
      }
    }
    return res;
  }

  void NotifyAll(Unit* unit, const UnitSet& enter_set,
                 const UnitSet& leave_set) const {
    NotifyEnter(unit, enter_set);
    NotifyLeave(unit, leave_set);
  }

  void NotifyEnter(Unit* unit, const UnitSet& enter_set) const {
    for (const auto& other : enter_set) {
      other->enter_callback(other->id, unit->id);
      other->Subscribe(unit);
      unit->enter_callback(unit->id, other->id);
      unit->Subscribe(other);
    }
  }

  void NotifyLeave(Unit* unit, const UnitSet& leave_set) const {
    for (const auto& other : leave_set) {
      other->leave_callback(other->id, unit->id);
      other->UnSubscribe(unit);
    }
  }

  const float width_;
  const float height_;
  const float visible_range_;

 private:
  mutable std::unordered_map<int, Unit*> unit_map_;
};

#endif  // AOI_H