#pragma once

template <class _From, class _To>
constexpr bool is_convertible_v = __is_convertible_to(_From, _To);

template <class _From, class _To>
concept convertible_to = is_convertible_v<_From, _To>;
