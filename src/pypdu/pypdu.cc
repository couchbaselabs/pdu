#include "pypdu.h"

#include <pdu/pdu.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

// Wrapper for a filter returned by a C++ method (e.g., pdu::filter::regex)
// to avoid overheads of calling through pybind. This distinguishes the
// callback from one which is genuinely python code.
struct WrappedFilter {
    WrappedFilter(const WrappedFilter&) = delete;
    WrappedFilter(WrappedFilter&&) = default;
    pdu::filter::Filter filter;
};

SeriesFilter makeFilter(const py::dict& dict) {
    SeriesFilter f;
    for (const auto& [kobj, vobj] : dict) {
        std::string k = py::str(kobj);
        if (py::isinstance<py::function>(vobj)) {
            // arbitrary python callback. Convert it to a Filter, which will
            // call back into Python on every evaluation (probably slow)
            f.addFilter(k, vobj.cast<pdu::filter::Filter>());
        } else if (py::isinstance<WrappedFilter>(vobj)) {
            // is a filter created in C++, get a reference to the real type
            f.addFilter(k, vobj.cast<WrappedFilter&>().filter);
        } else if (py::str(vobj)) {
            // is a plain old string, will filter to an exact match on this
            // label
            f.addFilter(k, std::string(py::str(vobj)));
        } else {
            throw std::invalid_argument(
                    "Filter only handles strings, regexes, and unary "
                    "funcs");
        }
    }
    return f;
}

/**
 * Make a filter from a single string.
 *
 * String is treated as an exact metric family name (__name__) to match.
 */
SeriesFilter makeFilter(const py::str& s) {
    SeriesFilter f;
    f.addFilter("__name__", std::string(s));
    return f;
}

/**
 * Make a filter from a single unary python function
 *
 * String is treated as a callable to match against a metric family name.
 */
SeriesFilter makeFilter(const pdu::filter::Filter& callable) {
    SeriesFilter f;
    f.addFilter("__name__", callable);
    return f;
}

/**
 * Make a filter from a single unary python function
 *
 * String is treated as a callable (regex or arbitrary python func) to match
 * against a metric family name.
 */
SeriesFilter makeFilter(const WrappedFilter& wf) {
    SeriesFilter f;
    f.addFilter("__name__", wf.filter);
    return f;
}

auto makeFilteredPyIterator(const PrometheusData& pd, const SeriesFilter& f) {
    return py::make_iterator<py::return_value_policy::copy,
                             SeriesIterator,
                             EndSentinel,
                             CrossIndexSeries>(pd.filtered(f), pd.end());
}

template <class T>
auto makeFilteredPyIterator(const PrometheusData& pd, const T& val) {
    return makeFilteredPyIterator(pd, makeFilter(val));
}

auto getFirstMatching(const PrometheusData& pd, const SeriesFilter& f) {
    auto itr = pd.filtered(f);
    if (itr == pd.end()) {
        throw py::key_error("No item matching filter");
    }
    return *itr;
}

template <class T>
auto getFirstMatching(const PrometheusData& pd, const T& val) {
    return getFirstMatching(pd, makeFilter(val));
}

PYBIND11_MODULE(pypdu, m) {
    m.doc() = "Python bindings to pdu, for reading Prometheus on-disk data";

    m.def("load",
          py::overload_cast<const std::string&>(&pdu::load),
          "Load data from a Prometheus data directory");

    m.def(
            "regex",
            [](std::string expression) {
                return WrappedFilter{pdu::filter::regex(expression)};
            },
            "Specify a regular expression for a Filter to match against "
            "label "
            "values");

    py::class_<WrappedFilter>(m, "FilterFunc");

    py::class_<SeriesFilter>(m, "Filter")
            .def(py::init<>())
            .def(py::init<>(
                    [](const py::dict& dict) { return makeFilter(dict); }))
            .def("add",
                 [](SeriesFilter& f,
                    const std::string& labelKey,
                    const std::string& labelValue) {
                     f.addFilter(labelKey, labelValue);
                 })
            .def(
                    "addRegex",
                    [](SeriesFilter& f,
                       const std::string& labelKey,
                       const std::string& labelValue) {
                        f.addFilter(labelKey, pdu::filter::regex(labelValue));
                    },
                    "Add a label filter which matches values against an "
                    "ECMAScript regex")
            .def("is_empty", [](const SeriesFilter& f) { return f.empty(); });

    py::class_<Sample>(m, "Sample")
            .def_readonly("timestamp", &Sample::timestamp)
            .def_readonly("value", &Sample::value)
            // support unpacking in the form of
            // for sample in samples:
            //     ...
            .def(
                    "__getitem__",
                    [](const Sample& sample, size_t i) {
                        if (i >= 2) {
                            throw py::index_error();
                        }
                        return i == 0 ? sample.timestamp : sample.value;
                    },
                    py::return_value_policy::copy)
            .def("__len__", []() { return 2; })
            // for nice presentation
            .def("__repr__",
                 [](const Sample& a) {
                     return "{timestamp=" + std::to_string(a.timestamp) +
                            ", value=" + std::to_string(a.value) + "}";
                 })
            .def(py::self == py::self)
            .def(py::self != py::self);

    // note - this is intentionally inconsistent naming to better reflect
    // the fact that in normal usage __iter__ will be called on this type
    // to get a python iterator, despite this being a C++ iterator already.
    py::class_<CrossIndexSampleIterator>(m, "CrossIndexSampleIterable")
            .def(
                    "__len__",
                    [](const CrossIndexSampleIterator& cisi) {
                        return cisi.getNumSamples();
                    },
                    py::return_value_policy::copy)
            .def(
                    "__iter__",
                    [](const CrossIndexSampleIterator& cisi) {
                        return py::make_iterator<py::return_value_policy::copy,
                                                 CrossIndexSampleIterator,
                                                 EndSentinel,
                                                 Sample>(cisi, end(cisi));
                    },
                    py::keep_alive<0, 1>());

    py::class_<CrossIndexSeries>(m, "Series")
            .def_property_readonly(
                    "name",
                    [](const CrossIndexSeries& cis) {
                        if (!cis.series) {
                            throw std::runtime_error(
                                    "Can't get name, series is invalid");
                        }
                        return cis.series->labels.at("__name__");
                    },
                    py::keep_alive<0, 1>())
            .def_property_readonly(
                    "labels",
                    [](const CrossIndexSeries& cis) {
                        if (!cis.series) {
                            throw std::runtime_error(
                                    "Can't get labels, series is invalid");
                        }
                        return cis.series->labels;
                    },
                    py::keep_alive<0, 1>())
            .def_property_readonly(
                    "samples",
                    [](const CrossIndexSeries& cis) {
                        return cis.sampleIterator;
                    },
                    py::keep_alive<0, 1>())
            // support unpacking in the form of
            // for series, samples in data:
            //     ...
            .def("__getitem__",
                 [](const CrossIndexSeries& cis, size_t i) {
                     if (!cis.series) {
                         throw std::runtime_error(
                                 "Can't unpack, series is invalid");
                     }
                     switch (i) {
                     case 0:
                         return py::cast(cis.series->labels.at("__name__"));
                     case 1:
                         return py::cast(cis.series->labels);
                     case 2:
                         auto ret = py::cast(cis.sampleIterator);
                         // manually set up keep alive here, as it is not
                         // possible to do so for the name and labels, so it
                         // cannot be set as a policy for the method.
                         keep_alive_impl(ret, py::cast(cis));
                         return ret;
                     }
                     throw py::index_error();
                 })
            .def("__len__", []() { return 3; });

    py::class_<PrometheusData>(m, "PrometheusData")
                    .def(py::init<std::string>())
    // Allow iteration, default to unfiltered (all time series will be listed)
    .def(
        "__iter__",
        [](const PrometheusData& pd) {
            return py::make_iterator<py::return_value_policy::copy,
                                     SeriesIterator,
                                     EndSentinel,
                                     CrossIndexSeries>(pd.begin(), pd.end());
        },
        py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */)
        // Allow iteration, default to unfiltered (all time series will be listed)
    .def(
        "filter",
        [](const PrometheusData& pd, const SeriesFilter& f) {
            return makeFilteredPyIterator(pd, f);
        },
        py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */)
    .def(
        "filter",
        [](const PrometheusData& pd, const py::dict& dict) {
            return makeFilteredPyIterator(pd, dict);
        },
        py::keep_alive<0, 1>())
    .def(
        "filter",
        [](const PrometheusData& pd, const py::str& s) {
            return makeFilteredPyIterator(pd, s);
        },
        py::keep_alive<0, 1>())
    // support an arbitrary python callback as a __name__ filter
    .def(
        "filter",
        [](const PrometheusData& pd, const pdu::filter::Filter& f) {
            return makeFilteredPyIterator(pd, f);
        },
        py::keep_alive<0, 1>())
    // support a C++ constructed filter (avoiding it being treated as a
    // python callback)
    .def(
        "filter",
        [](const PrometheusData& pd, const WrappedFilter& f) {
            return makeFilteredPyIterator(pd, f);
        },
        py::keep_alive<0, 1>())
    .def("__getitem__", [](const PrometheusData& pd, const SeriesFilter& f) {
        return getFirstMatching(pd, f);
        }, py::keep_alive<0, 1>())
    .def("__getitem__", [](const PrometheusData& pd, const py::dict& dict) {
        return getFirstMatching(pd, dict);
    }, py::keep_alive<0, 1>())
    .def("__getitem__", [](const PrometheusData& pd, const py::str& s) {
        return getFirstMatching(pd, s);
        }, py::keep_alive<0, 1>())
        // support an arbitrary python callback as a __name__ filter
    .def("__getitem__", [](const PrometheusData& pd, const pdu::filter::Filter& f) {
        return getFirstMatching(pd, f);
    }, py::keep_alive<0, 1>())
    // support a C++ constructed filter (avoiding it being treated as a
    // python callback)
    .def("__getitem__", [](const PrometheusData& pd, const WrappedFilter& f) {
        return getFirstMatching(pd, f);
    }, py::keep_alive<0, 1>());
}