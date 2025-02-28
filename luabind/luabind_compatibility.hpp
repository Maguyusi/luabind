#pragma once

// lua_strlen() and lua_equal() are used in Luabind, but these have been removed in Lua 5.4.
// Still we can enable the compatible macros by defining LUA_COMPAT_5_3 before including "luaconf.h".

#include "lua.hpp"

#if (LUA_VERSION_NUM >= 502)
// Lua 5.1 compatible functions.
inline void lua_setfenv(lua_State* L, int idx) { lua_setuservalue(L, idx); }
inline void lua_getfenv(lua_State* L, int idx) { lua_getuservalue(L, idx); }
inline lua_State* lua_open() { return luaL_newstate(); }

#if (LUA_VERSION_NUM >= 504)
inline int lua_resume(lua_State* L, int nargs)
{
	int nresults = 0;
	return lua_resume(L, nullptr, nargs, &nresults);
}
#else
inline int lua_resume(lua_State* L, int nargs) { return lua_resume(L, nullptr, nargs); }
#endif

struct DummyClassForLegacyLuaGlobalsIndex {};
const DummyClassForLegacyLuaGlobalsIndex LUA_GLOBALSINDEX;
inline void lua_pushvalue(lua_State* L, const DummyClassForLegacyLuaGlobalsIndex&) { lua_pushglobaltable(L); }
inline void lua_settable(lua_State* L, const DummyClassForLegacyLuaGlobalsIndex&) { lua_settable(L, LUA_REGISTRYINDEX); }
inline void lua_gettable(lua_State* L, const DummyClassForLegacyLuaGlobalsIndex&) { lua_gettable(L, LUA_REGISTRYINDEX); }
#else
// Lua 5.2 compatible functions.
inline void lua_pushglobaltable(lua_State* L) { lua_pushvalue(L, LUA_GLOBALSINDEX); }
#endif

//In the C++17 standard, official support for std::auto_ptr was removed, so we had to implement them manually.
//Also, Boost turns off support for std::auto_ptr under C++17, and we need to patch them in manually as well.
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
// Support for C++17
namespace std
{
	template <class _Ty>
	class auto_ptr;

	template <class _Ty>
	struct auto_ptr_ref { // proxy reference for auto_ptr copying
		explicit auto_ptr_ref(_Ty* _Right) : _Ref(_Right) {}

		_Ty* _Ref; // generic pointer to auto_ptr ptr
	};

	template <class _Ty>
	class auto_ptr { // wrap an object pointer to ensure destruction
	public:
		using element_type = _Ty;

		explicit auto_ptr(_Ty* _Ptr = nullptr) noexcept : _Myptr(_Ptr) {}

		auto_ptr(auto_ptr& _Right) noexcept : _Myptr(_Right.release()) {}

		auto_ptr(auto_ptr_ref<_Ty> _Right) noexcept {
			_Ty* _Ptr = _Right._Ref;
			_Right._Ref = nullptr; // release old
			_Myptr = _Ptr; // reset this
		}

		template <class _Other>
		operator auto_ptr<_Other>() noexcept { // convert to compatible auto_ptr
			return auto_ptr<_Other>(*this);
		}

		template <class _Other>
		operator auto_ptr_ref<_Other>() noexcept { // convert to compatible auto_ptr_ref
			_Other* _Cvtptr = _Myptr; // test implicit conversion
			auto_ptr_ref<_Other> _Ans(_Cvtptr);
			_Myptr = nullptr; // pass ownership to auto_ptr_ref
			return _Ans;
		}

		template <class _Other>
		auto_ptr& operator=(auto_ptr<_Other>& _Right) noexcept {
			reset(_Right.release());
			return *this;
		}

		template <class _Other>
		auto_ptr(auto_ptr<_Other>& _Right) noexcept : _Myptr(_Right.release()) {}

		auto_ptr& operator=(auto_ptr& _Right) noexcept {
			reset(_Right.release());
			return *this;
		}

		auto_ptr& operator=(auto_ptr_ref<_Ty> _Right) noexcept {
			_Ty* _Ptr = _Right._Ref;
			_Right._Ref = 0; // release old
			reset(_Ptr); // set new
			return *this;
		}

		~auto_ptr() noexcept {
			delete _Myptr;
		}

		_NODISCARD _Ty& operator*() const noexcept {
			return *get();
		}

		_NODISCARD _Ty* operator->() const noexcept {
			return get();
		}

		_NODISCARD _Ty* get() const noexcept {
			return _Myptr;
		}

		_Ty* release() noexcept {
			_Ty* _Tmp = _Myptr;
			_Myptr = nullptr;
			return _Tmp;
		}

		void reset(_Ty* _Ptr = nullptr) noexcept { // destroy designated object and store new pointer
			if (_Ptr != _Myptr) {
				delete _Myptr;
			}

			_Myptr = _Ptr;
		}

	private:
		_Ty* _Myptr; // the wrapped object pointer
	};

	template <>
	class auto_ptr<void> {
	public:
		using element_type = void;
	};
}
namespace boost
{
	template<class T> T* get_pointer(std::auto_ptr<T> const& p)
	{
		return p.get();
	}
}
#endif

// Suppress automatic inclusion of <boost/bind/placeholders.hpp> in <boost/bind/bind.hpp> in order to avoid conflict between Boost and C++11 placeholders.
#define BOOST_BIND_NO_PLACEHOLDERS

// Luabind 0.9.1 uses "boost::operator" but it has been moved to "boost::iterators::operator" in Boost 1.57.0 or later.
// As a result, many compilation errors will occur at the macro "LUABIND_OPERATOR_ADL_WKND" in "luabind/object.hpp".
// One of the best and wisest solutions is to modify the source code of Luabind directly.
// As an alternative way, the following workaround can avoid modifying it but is unbeautiful and pollutes the namespace "boost".
#include <boost/version.hpp>
#if (defined(BOOST_VERSION) && BOOST_VERSION >= 105700)
#include <boost/iterator/iterator_facade.hpp>
namespace luabind
{
	namespace detail
	{
		// Forward declaration
		template<typename T> class basic_iterator;
	}
}
namespace boost
{
	template<typename T> bool operator ==(
		const luabind::detail::basic_iterator<T>& x,
		const luabind::detail::basic_iterator<T>& y)
	{
		return boost::iterators::operator ==(x, y);
	}

	template<typename T> bool operator !=(
		const luabind::detail::basic_iterator<T>& x,
		const luabind::detail::basic_iterator<T>& y)
	{
		return boost::iterators::operator !=(x, y);
	}
}
#endif
