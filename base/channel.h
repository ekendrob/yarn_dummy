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
  virtual size_t Size_() const = 0;
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
  virtual size_t Size_() const = 0;
  virtual std::string Name_() const = 0;
};

template <typename DataType>
class Channel : public ChannelPut<DataType>, public ChannelGet<DataType> {
 public:
  Channel(const std::string& name, uint64_t depth = std::numeric_limits<uint64_t>::max())
      : name_(name),
        depth_(depth),
        get_event_(std::string(name + "_get_event").c_str()),
        put_event_(std::string(name + "_put_event").c_str()) {};

  virtual ~Channel() = default;
  size_t Size() { return ChannelPut<DataType>::Size(); }  // Note! Either one works

 private:
  const std::string name_;
  const uint64_t depth_;
  std::deque<DataType> storage_;
  sc_event get_event_;
  sc_event put_event_;

  void Put_(const DataType& d) {
    if (Size() >= depth_) wait(get_event_);
    storage_.push_back(d);
    put_event_.notify();
  }

  DataType Get_() {
    while (!Size()) wait(put_event_);
    DataType returned(storage_.front());
    storage_.pop_front();
    get_event_.notify();
    return returned;
  }

  std::string Name_() const { return name_; }
  size_t Size_() const { return storage_.size(); }
};

}  // namespace yarn

#endif  // YARN_BASE_CHANNEL_H_