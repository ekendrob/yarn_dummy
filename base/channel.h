#ifndef YARN_BASE_CHANNEL_H_
#define YARN_BASE_CHANNEL_H_

//  Note! This is inspired by
//  [truss_channel.h](https://github.com/trusster/trusster/blob/master/truss/cpp/inc/truss_channel.h)

namespace yarn {

template <class data_type>
class channel_put {
 public:
  virtual ~channel_put() = default;
  void put(const data_type& d) { put_(d); }
  size_t size() { return size_(); }
  std::string name() { return name_(); }

 protected:
  virtual void put_(const data_type& d) = 0;
  virtual size_t size_() = 0;
  virtual std::string name_() const = 0;
};

template <class data_type>
class channel_get {
 public:
  virtual ~channel_get() = default;
  data_type get() { return get_(); }
  size_t size() { return size_(); }
  std::string name() { return name_(); }

 protected:
  virtual data_type get_() = 0;
  virtual size_t size_() = 0;
  virtual std::string name_() const = 0;
};

//FIXME: Implement channel class combining truss_channel.h with SystemC Event

}  // namespace yarn

#endif  // YARN_BASE_CHANNEL_H_