#pragma once
#include <iterator>

struct curl_slist;
namespace asyncpp::curl {
	class slist;
	class handle;
	class slist_iterator {
		curl_slist* m_current = nullptr;
		friend class slist;

	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t; // This is useless, but c++ requires it
		using value_type = char*;
		using pointer = value_type*;
		using reference = value_type&;

		slist_iterator(curl_slist* elem = nullptr) : m_current{elem} {}
		slist_iterator(const slist_iterator&) = default;
		slist_iterator& operator=(const slist_iterator&) = default;

		reference operator*() const noexcept;
		pointer operator->() const noexcept;
		slist_iterator& operator++() noexcept;
		slist_iterator operator++(int) noexcept;
		friend bool operator==(const slist_iterator& a, const slist_iterator& b) { return a.m_current == b.m_current; }
		friend bool operator!=(const slist_iterator& a, const slist_iterator& b) { return a.m_current != b.m_current; }
	};

	class slist {
		curl_slist* m_first_node = nullptr;
		curl_slist* m_last_node = nullptr; // Used to speed up append
		friend class handle;

	public:
		static const struct ownership_take_tag {
		} ownership_take;
		static const struct ownership_copy_tag {
		} ownership_copy;
		slist() = default;
		// Takes ownership
		slist(curl_slist* raw, ownership_take_tag);
		slist(const curl_slist* raw, ownership_copy_tag);
		slist(const slist& other);
		slist& operator=(const slist& other);
		slist(slist&& other);
		slist& operator=(slist&& other);
		~slist() noexcept { clear(); }

		slist_iterator append(const char* str);
		slist_iterator prepend(const char* str);
		slist_iterator insert(size_t index, const char* str);
		slist_iterator insert_after(slist_iterator it, const char* str);
		void remove(size_t index);
		void remove(slist_iterator it);
		void clear();
		bool empty() const noexcept { return m_first_node == nullptr; }

		auto begin() const noexcept { return slist_iterator{m_first_node}; }
		auto end() const noexcept { return slist_iterator{nullptr}; }
	};
} // namespace asyncpp::curl