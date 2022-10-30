#pragma once
#include <curl/curl.h>
#include <iterator>
#include <utility>

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

		constexpr explicit slist_iterator(curl_slist* elem = nullptr) : m_current{elem} {}

		reference operator*() const noexcept { return m_current->data; }
		pointer operator->() const noexcept { return &m_current->data; }
		slist_iterator& operator++() noexcept {
			if (m_current != nullptr) m_current = m_current->next;
			return *this;
		}
		slist_iterator operator++(int) noexcept {
			slist_iterator tmp = *this;
			++(*this);
			return tmp;
		}
		friend bool operator==(const slist_iterator& a, const slist_iterator& b) { return a.m_current == b.m_current; }
		friend bool operator!=(const slist_iterator& a, const slist_iterator& b) { return a.m_current != b.m_current; }
	};

	class slist {
		curl_slist* m_first_node = nullptr;
		curl_slist* m_last_node = nullptr; // Used to speed up append
		friend class handle;

	public:
		inline static const struct ownership_take_tag {
		} ownership_take;
		inline static const struct ownership_copy_tag {
		} ownership_copy;
		constexpr slist() = default;
		// Takes ownership
		constexpr slist(curl_slist* raw, ownership_take_tag) noexcept : m_first_node{raw} {
			while (raw != nullptr && raw->next != nullptr)
				raw = raw->next;
			m_last_node = raw;
		}
		slist(const curl_slist* raw, ownership_copy_tag);
		slist(const slist& other);
		slist& operator=(const slist& other);
		constexpr slist(slist&& other) noexcept : m_first_node{other.m_first_node}, m_last_node{other.m_last_node} {
			other.m_first_node = nullptr;
			other.m_last_node = nullptr;
		}
		constexpr slist& operator=(slist&& other) noexcept {
			m_first_node = std::exchange(other.m_first_node, m_first_node);
			m_last_node = std::exchange(other.m_last_node, m_last_node);
			return *this;
		}
		~slist() noexcept { clear(); }

		slist_iterator append(const char* str) { return insert_after(end(), str); }
		slist_iterator prepend(const char* str);
		[[nodiscard]] constexpr slist_iterator index(size_t idx) const noexcept {
			auto node = m_first_node;
			for (size_t i = 0; i != idx && node != nullptr; i++)
				node = node->next;
			return slist_iterator{node};
		}
		slist_iterator insert(size_t idx, const char* str) {
			if (idx == 0) return prepend(str);
			return insert_after(index(idx - 1), str);
		}
		slist_iterator insert_after(slist_iterator it, const char* str);
		void remove(size_t index);
		void remove(slist_iterator it);
		void clear();
		[[nodiscard]] constexpr bool empty() const noexcept { return m_first_node == nullptr; }

		[[nodiscard]] constexpr slist_iterator begin() const noexcept { return slist_iterator{m_first_node}; }
		[[nodiscard]] constexpr slist_iterator end() const noexcept { return slist_iterator{nullptr}; }

		[[nodiscard]] constexpr curl_slist* release() noexcept {
			m_last_node = nullptr;
			return std::exchange(m_first_node, nullptr);
		}
	};
} // namespace asyncpp::curl
