/*******************************************************************************
 * Copyright 2017-2022 MINRES Technologies GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/
/*
 * report.cpp
 *
 *  Created on: 19.09.2017
 *      Author: eyck@minres.com
 */

#include <scp/report.h>
#include <set>
#include <map>
#include <array>
#include <chrono>
#include <fstream>
#include <systemc>
#ifdef HAS_CCI
#include <cci_configuration>
#endif
#include <mutex>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <tuple>
#include <unordered_map>
#if defined(__GNUC__) || defined(__clang__)
#define likely(x)   __builtin_expect(x, 1)
#define unlikely(x) __builtin_expect(x, 0)
#else
#define likely(x)   x
#define unlikely(x) x
#endif

#include <regex>
#ifdef ERROR
#undef ERROR
#endif

namespace {
// Making this thread_local could cause thread copies of the same cache
// entries, but more likely naming will be thread local too, and this avoids
// races in the unordered_map

#ifdef DISABLE_REPORT_THREAD_LOCAL
std::unordered_map<uint64_t, sc_core::sc_verbosity> lut;
#else
thread_local std::unordered_map<uint64_t, sc_core::sc_verbosity> lut;
#endif

#ifdef HAS_CCI
cci::cci_originator scp_global_originator("scp_reporting_global");
#endif

std::set<std::string> logging_parameters;

struct ExtLogConfig : public scp::LogConfig {
    std::shared_ptr<spdlog::logger> file_logger;
    std::shared_ptr<spdlog::logger> console_logger;
    std::regex reg_ex;
    sc_core::sc_time cycle_base{ 0, sc_core::SC_NS };
    auto operator=(const scp::LogConfig& o) -> ExtLogConfig& {
        scp::LogConfig::operator=(o);
        return *this;
    }
    auto match(const char* type) -> bool { return regex_search(type, reg_ex); }
};

/* normally put the config in thread local. If two threads try to use logging
 * they would both need to init the config from both threads. The alternative
 * is to switch on this define, but then care has to be taken not to
 * (re)initialize from different threads which would then be unsafe.*/
#ifdef DISABLE_REPORT_THREAD_LOCAL
ExtLogConfig log_cfg;
#else
thread_local ExtLogConfig log_cfg;
#endif

inline std::string padded(std::string str, size_t width,
                          bool show_ellipsis = true) {
    if (width < 7)
        return str;
    if (str.length() > width) {
        if (show_ellipsis) {
            auto pos = str.size() - (width - 6);
            return str.substr(0, 3) + "..." +
                   str.substr(pos, str.size() - pos);
        } else
            return str.substr(0, width);
    } else {
        return str + std::string(width - str.size(), ' ');
    }
}

auto get_tuple(const sc_core::sc_time& t)
    -> std::tuple<sc_core::sc_time::value_type, sc_core::sc_time_unit> {
    auto val = t.value();
    auto tr = (uint64_t)(sc_core::sc_time::from_value(1).to_seconds() * 1E15);
    auto scale = 0U;
    while ((tr % 10) == 0) {
        tr /= 10;
        scale++;
    }
    sc_assert(tr == 1);

    auto tu = scale / 3;
    while (tu < sc_core::SC_SEC && (val % 10) == 0) {
        val /= 10;
        scale++;
        tu += (0 == (scale % 3));
    }
    for (scale %= 3; scale != 0; scale--)
        val *= 10;
    return std::make_tuple(val, static_cast<sc_core::sc_time_unit>(tu));
}

auto time2string(const sc_core::sc_time& t) -> std::string {
    const std::array<const char*, 6> time_units{ "fs", "ps", "ns",
                                                 "us", "ms", "s " };
    const std::array<uint64_t, 6> multiplier{ 1ULL,
                                              1000ULL,
                                              1000ULL * 1000,
                                              1000ULL * 1000 * 1000,
                                              1000ULL * 1000 * 1000 * 1000,
                                              1000ULL * 1000 * 1000 * 1000 *
                                                  1000 };
    std::ostringstream oss;
    if (!t.value()) {
        oss << "0 s ";
    } else {
        const auto tt = get_tuple(t);
        const auto val = std::get<0>(tt);
        const auto scale = std::get<1>(tt);
        const auto fs_val = val * multiplier[scale];
        for (int j = multiplier.size() - 1; j >= scale; --j) {
            if (fs_val >= multiplier[j]) {
                const auto i = val / multiplier[j - scale];
                const auto f = val % multiplier[j - scale];
                oss << i << '.' << std::setw(3 * (j - scale))
                    << std::setfill('0') << std::right << f << ' '
                    << time_units[j];
                break;
            }
        }
    }
    return oss.str();
}
auto compose_message(const sc_core::sc_report& rep, const scp::LogConfig& cfg)
    -> const std::string {
    if (rep.get_severity() > sc_core::SC_INFO ||
        cfg.log_filter_regex.length() == 0 ||
        rep.get_verbosity() == sc_core::SC_MEDIUM ||
        log_cfg.match(rep.get_msg_type())) {
        std::stringstream os;
        if (likely(cfg.print_sim_time)) {
            if (unlikely(log_cfg.cycle_base.value())) {
                if (unlikely(cfg.print_delta))
                    os << "[" << std::setw(7) << std::setfill(' ')
                       << sc_core::sc_time_stamp().value() /
                              log_cfg.cycle_base.value()
                       << "(" << std::setw(5) << sc_core::sc_delta_count()
                       << ")]";
                else
                    os << "[" << std::setw(7) << std::setfill(' ')
                       << sc_core::sc_time_stamp().value() /
                              log_cfg.cycle_base.value()
                       << "]";
            } else {
                auto t = time2string(sc_core::sc_time_stamp());
                if (unlikely(cfg.print_delta))
                    os << "[" << std::setw(20) << std::setfill(' ') << t << "("
                       << std::setw(5) << sc_core::sc_delta_count() << ")]";
                else
                    os << "[" << std::setw(20) << std::setfill(' ') << t
                       << "]";
            }
        }
        if (unlikely(rep.get_id() >= 0))
            os << "("
               << "IWEF"[rep.get_severity()] << rep.get_id() << ") "
               << rep.get_msg_type() << ": ";
        else if (cfg.msg_type_field_width) {
            if (cfg.msg_type_field_width ==
                std::numeric_limits<unsigned>::max())
                os << rep.get_msg_type() << ": ";
            else
                os << padded(rep.get_msg_type(), cfg.msg_type_field_width)
                   << ": ";
        }
        if (*rep.get_msg())
            os << rep.get_msg();
        if (rep.get_severity() >= cfg.file_info_from) {
            if (rep.get_line_number())
                os << "\n         [FILE:" << rep.get_file_name() << ":"
                   << rep.get_line_number() << "]";
            sc_core::sc_simcontext* simc = sc_core::sc_get_curr_simcontext();
            if (simc && sc_core::sc_is_running()) {
                const char* proc_name = rep.get_process_name();
                if (proc_name)
                    os << "\n         [PROCESS:" << proc_name << "]";
            }
        }
        return os.str();
    } else
        return "";
}

inline auto get_verbosity(const sc_core::sc_report& rep) -> int {
    return rep.get_verbosity() > sc_core::SC_NONE &&
                   rep.get_verbosity() < sc_core::SC_LOW
               ? rep.get_verbosity() * 10
               : rep.get_verbosity();
}

inline void log2logger(spdlog::logger& logger, const sc_core::sc_report& rep,
                       const scp::LogConfig& cfg) {
    auto msg = compose_message(rep, cfg);
    if (!msg.size())
        return;
    switch (rep.get_severity()) {
    case sc_core::SC_INFO:
        switch (get_verbosity(rep)) {
        case sc_core::SC_DEBUG:
        case sc_core::SC_FULL:
            logger.trace(msg);
            break;
        case sc_core::SC_HIGH:
            logger.debug(msg);
            break;
        default:
            logger.info(msg);
            break;
        }
        break;
    case sc_core::SC_WARNING:
        logger.warn(msg);
        break;
    case sc_core::SC_ERROR:
        logger.error(msg);
        break;
    case sc_core::SC_FATAL:
        logger.critical(msg);
        break;
    default:
        break;
    }
}

inline void log2logger(spdlog::logger& logger, scp::log lvl,
                       const std::string& msg) {
    switch (lvl) {
    case scp::log::DBGTRACE:
    case scp::log::TRACE:
        logger.trace(msg);
        return;
    case scp::log::DEBUG:
        logger.debug(msg);
        return;
    case scp::log::INFO:
        logger.info(msg);
        return;
    case scp::log::WARNING:
        logger.warn(msg);
        return;
    case scp::log::ERROR:
        logger.error(msg);
        return;
    case scp::log::FATAL:
        logger.critical(msg);
        return;
    default:
        break;
    }
}

void report_handler(const sc_core::sc_report& rep,
                    const sc_core::sc_actions& actions) {
    thread_local bool sc_stop_called = false;
    if (actions & sc_core::SC_DO_NOTHING)
        return;
    if (rep.get_severity() == sc_core::SC_INFO ||
        !log_cfg.report_only_first_error ||
        sc_core::sc_report_handler::get_count(sc_core::SC_ERROR) < 2) {
        if ((actions & sc_core::SC_DISPLAY) &&
            (!log_cfg.file_logger || get_verbosity(rep) < sc_core::SC_HIGH))
            log2logger(*log_cfg.console_logger, rep, log_cfg);
        if ((actions & sc_core::SC_LOG) && log_cfg.file_logger) {
            scp::LogConfig lcfg(log_cfg);
            lcfg.print_sim_time = true;
            if (!lcfg.msg_type_field_width)
                lcfg.msg_type_field_width = 24;
            log2logger(*log_cfg.file_logger, rep, lcfg);
        }
    }
    if (actions & sc_core::SC_STOP) {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<unsigned>(log_cfg.level) * 10));
        if (sc_core::sc_is_running() && !sc_stop_called) {
            sc_core::sc_stop();
            sc_stop_called = true;
        }
    }
    if (actions & sc_core::SC_ABORT) {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<unsigned>(log_cfg.level) * 20));
        abort();
    }
    if (actions & sc_core::SC_THROW) {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<unsigned>(log_cfg.level) * 20));
        throw rep;
    }
    if (sc_core::sc_time_stamp().value() && !sc_core::sc_is_running()) {
        log_cfg.console_logger->flush();
        if (log_cfg.file_logger)
            log_cfg.file_logger->flush();
    }
}

// BKDR hash algorithm
auto char_hash(char const* str) -> uint64_t {
    constexpr unsigned int seed = 131; // 31  131 1313 13131131313 etc//
    uint64_t hash = 0;
    while (*str) {
        hash = (hash * seed) + (*str);
        str++;
    }
    return hash;
}
} // namespace

static const std::array<sc_core::sc_severity, 8> severity = {
    sc_core::SC_FATAL,   // scp::log::NONE
    sc_core::SC_FATAL,   // scp::log::FATAL
    sc_core::SC_ERROR,   // scp::log::ERROR
    sc_core::SC_WARNING, // scp::log::WARNING
    sc_core::SC_INFO,    // scp::log::INFO
    sc_core::SC_INFO,    // scp::log::DEBUG
    sc_core::SC_INFO,    // scp::log::TRACE
    sc_core::SC_INFO     // scp::log::TRACEALL
};
static const std::array<sc_core::sc_verbosity, 8> verbosity = {
    sc_core::SC_NONE,   // scp::log::NONE
    sc_core::SC_LOW,    // scp::log::FATAL
    sc_core::SC_LOW,    // scp::log::ERROR
    sc_core::SC_LOW,    // scp::log::WARNING
    sc_core::SC_MEDIUM, // scp::log::INFO
    sc_core::SC_HIGH,   // scp::log::DEBUG
    sc_core::SC_FULL,   // scp::log::TRACE
    sc_core::SC_DEBUG   // scp::log::TRACEALL
};
static std::mutex cfg_guard;
static void configure_logging() {
    std::lock_guard<std::mutex> lock(cfg_guard);
    static bool spdlog_initialized = false;

    sc_core::sc_report_handler::set_actions(
        sc_core::SC_ERROR,
        sc_core::SC_DEFAULT_ERROR_ACTIONS | sc_core::SC_DISPLAY);
    sc_core::sc_report_handler::set_actions(sc_core::SC_FATAL,
                                            sc_core::SC_DEFAULT_FATAL_ACTIONS);
    sc_core::sc_report_handler::set_verbosity_level(
        verbosity[static_cast<unsigned>(log_cfg.level)]);
    sc_core::sc_report_handler::set_handler(report_handler);
    if (!spdlog_initialized) {
        spdlog::init_thread_pool(
            1024U,
            log_cfg.log_file_name.size()
                ? 2U
                : 1U); // queue with 8k items and 1 backing thread.
        log_cfg.console_logger = log_cfg.log_async
                                     ? spdlog::stdout_color_mt<
                                           spdlog::async_factory>(
                                           "console_logger")
                                     : spdlog::stdout_color_mt(
                                           "console_logger");
        auto logger_fmt = log_cfg.print_severity ? "[%L] %v" : "%v";
        if (log_cfg.colored_output) {
            std::ostringstream os;
            os << "%^" << logger_fmt << "%$";
            log_cfg.console_logger->set_pattern(os.str());
        } else
            log_cfg.console_logger->set_pattern("[%L] %v");
        log_cfg.console_logger->flush_on(spdlog::level::warn);
        log_cfg.console_logger->set_level(spdlog::level::level_enum::trace);
        if (log_cfg.log_file_name.size()) {
            {
                std::ofstream ofs;
                ofs.open(log_cfg.log_file_name,
                         std::ios::out | std::ios::trunc);
            }
            log_cfg.file_logger = log_cfg.log_async
                                      ? spdlog::basic_logger_mt<
                                            spdlog::async_factory>(
                                            "file_logger",
                                            log_cfg.log_file_name)
                                      : spdlog::basic_logger_mt(
                                            "file_logger",
                                            log_cfg.log_file_name);
            if (log_cfg.print_severity)
                log_cfg.file_logger->set_pattern("[%8l] %v");
            else
                log_cfg.file_logger->set_pattern("%v");
            log_cfg.file_logger->flush_on(spdlog::level::warn);
            log_cfg.file_logger->set_level(spdlog::level::level_enum::trace);
        }
        spdlog_initialized = true;
    } else {
        log_cfg.console_logger = spdlog::get("console_logger");
        if (log_cfg.log_file_name.size())
            log_cfg.file_logger = spdlog::get("file_logger");
    }
    if (log_cfg.log_filter_regex.size()) {
        log_cfg.reg_ex = std::regex(log_cfg.log_filter_regex,
                                    std::regex::extended | std::regex::icase);
    }
}

void scp::reinit_logging(scp::log level) {
    sc_core::sc_report_handler::set_handler(report_handler);
    log_cfg.level = level;
    lut.clear();
}

void scp::init_logging(scp::log level, unsigned type_field_width,
                       bool print_time) {
    log_cfg.msg_type_field_width = type_field_width;
    log_cfg.print_sys_time = print_time;
    log_cfg.level = level;
    configure_logging();
}

void scp::init_logging(const scp::LogConfig& log_config) {
    log_cfg = log_config;
    configure_logging();
}

void scp::set_logging_level(scp::log level) {
    log_cfg.level = level;
    sc_core::sc_report_handler::set_verbosity_level(
        verbosity[static_cast<unsigned>(level)]);
    log_cfg.console_logger->set_level(static_cast<spdlog::level::level_enum>(
        SPDLOG_LEVEL_OFF -
        std::min<int>(SPDLOG_LEVEL_OFF, static_cast<int>(log_cfg.level))));
}

auto scp::get_logging_level() -> scp::log {
    return log_cfg.level;
}

void scp::set_cycle_base(sc_core::sc_time period) {
    log_cfg.cycle_base = period;
}

auto scp::LogConfig::logLevel(scp::log level) -> scp::LogConfig& {
    this->level = level;
    return *this;
}

auto scp::LogConfig::msgTypeFieldWidth(unsigned width) -> scp::LogConfig& {
    this->msg_type_field_width = width;
    return *this;
}

auto scp::LogConfig::printSysTime(bool enable) -> scp::LogConfig& {
    this->print_sys_time = enable;
    return *this;
}

auto scp::LogConfig::printSimTime(bool enable) -> scp::LogConfig& {
    this->print_sim_time = enable;
    return *this;
}

auto scp::LogConfig::printDelta(bool enable) -> scp::LogConfig& {
    this->print_delta = enable;
    return *this;
}

auto scp::LogConfig::printSeverity(bool enable) -> scp::LogConfig& {
    this->print_severity = enable;
    return *this;
}

auto scp::LogConfig::logFileName(std::string&& name) -> scp::LogConfig& {
    this->log_file_name = name;
    return *this;
}

auto scp::LogConfig::logFileName(const std::string& name) -> scp::LogConfig& {
    this->log_file_name = name;
    return *this;
}

auto scp::LogConfig::coloredOutput(bool enable) -> scp::LogConfig& {
    this->colored_output = enable;
    return *this;
}

auto scp::LogConfig::logFilterRegex(std::string&& expr) -> scp::LogConfig& {
    this->log_filter_regex = expr;
    return *this;
}

auto scp::LogConfig::logFilterRegex(const std::string& expr)
    -> scp::LogConfig& {
    this->log_filter_regex = expr;
    return *this;
}

auto scp::LogConfig::logAsync(bool v) -> scp::LogConfig& {
    this->log_async = v;
    return *this;
}

auto scp::LogConfig::reportOnlyFirstError(bool v) -> scp::LogConfig& {
    this->report_only_first_error = v;
    return *this;
}

auto scp::LogConfig::fileInfoFrom(int v) -> scp::LogConfig& {
    this->file_info_from = v;
    return *this;
}

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, '.')) {
        result.push_back(item);
    }
    return result;
}

std::string join(std::vector<std::string> vec) {
    if (vec.empty())
        return "";
    return std::accumulate(
        vec.begin(), vec.end(), std::string(),
        [](const std::string& a, const std::string& b) -> std::string {
            return a + (a.length() > 0 ? "." : "") + b;
        });
}

std::vector<std::string> scp::get_logging_parameters() {
    return std::vector<std::string>(logging_parameters.begin(),
                                    logging_parameters.end());
}

#ifdef HAS_CCI
sc_core::sc_verbosity cci_lookup(cci::cci_broker_handle broker,
                                 std::string name) {
    auto param_name = (name.empty()) ? SCP_LOG_LEVEL_PARAM_NAME
                                     : name + "." SCP_LOG_LEVEL_PARAM_NAME;
    auto h = broker.get_param_handle(param_name);
    if (h.is_valid()) {
        return verbosity.at(std::min<unsigned>(h.get_cci_value().get_int(),
                                               verbosity.size() - 1));
    } else {
        auto val = broker.get_preset_cci_value(param_name);

        if (val.is_int()) {
            broker.lock_preset_value(param_name);
            return verbosity.at(
                std::min<unsigned>(val.get_int(), verbosity.size() - 1));
        }
    }
    return sc_core::SC_UNSET;
}
#endif

#ifdef __GNUG__
std::string demangle(const char* name) {
    int status = -4; // some arbitrary value to eliminate the compiler
                     // warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void (*)(void*)> res{
        abi::__cxa_demangle(name, NULL, NULL, &status), std::free
    };

    return (status == 0) ? res.get() : name;
}
#else
// does nothing if not GNUG
std::string demangle(const char* name) {
    return name;
}
#endif

void insert(std::multimap<int, std::string, std::greater<int>>& map,
            std::string s, bool interesting) {
    int n = std::count(s.begin(), s.end(), '.');
    map.insert(make_pair(n, s));

    if (interesting) {
        logging_parameters.insert(s + "." SCP_LOG_LEVEL_PARAM_NAME);
    }
}

sc_core::sc_verbosity scp::scp_logger_cache::get_log_verbosity_cached(
    const char* scname, const char* tname = "") {
    if (level != sc_core::SC_UNSET) {
        return level;
    }

    if (!scname && features.size())
        scname = features[0].c_str();
    if (!scname)
        scname = "";

    type = std::string(scname);

#ifdef HAS_CCI
    try {
        // we rely on there being a broker, allow this to throw if not
        auto broker = sc_core::sc_get_current_object()
                          ? cci::cci_get_broker()
                          : cci::cci_get_global_broker(scp_global_originator);

        std::multimap<int, std::string, std::greater<int>> allfeatures;

        /* initialize */
        for (auto scn = split(scname); scn.size(); scn.pop_back()) {
            for (int first = 0; first < scn.size(); first++) {
                auto f = scn.begin() + first;
                std::vector<std::string> p(f, scn.end());
                auto scn_str = ((first > 0) ? "*." : "") + join(p);

                for (auto ft : features) {
                    for (auto ftn = split(ft); ftn.size(); ftn.pop_back()) {
                        insert(allfeatures, scn_str + "." + join(ftn),
                               first == 0);
                    }
                }
                insert(allfeatures, scn_str + "." + demangle(tname),
                       first == 0);
                insert(allfeatures, scn_str, first == 0);
            }
        }
        for (auto ft : features) {
            for (auto ftn = split(ft); ftn.size(); ftn.pop_back()) {
                insert(allfeatures, join(ftn), true);
                insert(allfeatures, "*." + join(ftn), false);
            }
        }
        insert(allfeatures, demangle(tname), true);
        insert(allfeatures, "*", false);
        insert(allfeatures, "", false);

        for (std::pair<int, std::string> f : allfeatures) {
            sc_core::sc_verbosity v = cci_lookup(broker, f.second);
            if (v != sc_core::SC_UNSET) {
                level = v;
                return v;
            }
        }
    } catch (const std::exception&) {
        // If there is no global broker, revert to initialized verbosity level
    }

#endif

    return level = static_cast<sc_core::sc_verbosity>(
               ::sc_core::sc_report_handler::get_verbosity_level());
}

auto scp::get_log_verbosity(char const* str) -> sc_core::sc_verbosity {
    auto k = char_hash(str);
    auto it = lut.find(k);
    if (it != lut.end())
        return it->second;

    scp::scp_logger_cache tmp;
    lut[k] = tmp.get_log_verbosity_cached(str);
    return lut[k];
}
