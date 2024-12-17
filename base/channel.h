#ifndef YARN_BASE_CHANNEL_H_
#define YARN_BASE_CHANNEL_H_

//  Note! This is inspired by
//  [truss_channel.h](https://github.com/trusster/trusster/blob/master/truss/cpp/inc/truss_channel.h)

namespace yarn {

template <class data_type>
class ChannelPut {
 public:
  virtual ~ChannelPut() = default;
  void Put(const data_type& d) { Put_(d); }
  size_t Size() { return Size_(); }
  std::string Name() { return Name_(); }

 protected:
  virtual void Put_(const data_type& d) = 0;
  virtual size_t Size_() = 0;
  virtual std::string Name_() const = 0;
};

template <class data_type>
class ChannelGet {
 public:
  virtual ~ChannelGet() = default;
  data_type Get() { return Get_(); }
  size_t Size() { return Size_(); }
  std::string Name() { return Name_(); }

 protected:
  virtual data_type Get_() = 0;
  virtual size_t Size_() = 0;
  virtual std::string Name_() const = 0;
};

template <typename data_type>
class GTestChannel : public ChannelGet<data_type> {
 public:
  GTestChannel(const std::string& n) : name_(n) {};
  virtual ~GTestChannel() = default;

  data_type Get_() {
    // FIXME: Return a specific scenario for testing here
    data_type returned;
    return returned;
  }
  size_t Size_() { return -1; }
  std::string Name_() const { return name_; }

 private:
  std::string name_;
};

// FIXME: Implement real channel with put and get inheritance

}  // namespace yarn

#endif  // YARN_BASE_CHANNEL_H_