#include <asyncpp/curl/slist.h>
#include <utility>
#include <new>

namespace asyncpp::curl {

	static std::pair<curl_slist*, curl_slist*> copy_slist(const curl_slist* ptr) {
		curl_slist* first_node = nullptr;
		curl_slist* last_node = nullptr;
		while (ptr) {
			last_node = curl_slist_append(last_node, ptr->data);
			if (!last_node) {
				// We failed to allocate the next node, clean up our new nodes and restore the original
				curl_slist_free_all(first_node);
				throw std::bad_alloc();
			}
			if (!first_node) first_node = last_node;
			ptr = ptr->next;
		}
		return {first_node, last_node};
	}

	slist::slist(const curl_slist* raw, ownership_copy_tag) {
		auto l = copy_slist(raw);
		m_first_node = l.first;
		m_last_node = l.second;
	}

	slist::slist(const slist& other) {
		auto l = copy_slist(other.m_first_node);
		m_first_node = l.first;
		m_last_node = l.second;
	}

	slist& slist::operator=(const slist& other) {
		// Copy list
		auto l = copy_slist(other.m_first_node);
		// We successfully made a copy, clean up the old list
		curl_slist_free_all(m_first_node);
		m_first_node = l.first;
		m_last_node = l.second;
		return *this;
	}

	slist_iterator slist::prepend(const char* str) {
		// We use curl_slist_append to alloc the new node because on windows a dll might use a different heap.
		auto node = curl_slist_append(nullptr, str);
		if (node == nullptr) throw std::bad_alloc{};
		node->next = m_first_node;
		m_first_node = node;
		if (m_last_node == nullptr) m_last_node = node;
		return slist_iterator{node};
	}

	slist_iterator slist::insert_after(slist_iterator pos, const char* str) {
		// We use curl_slist_append to alloc the new node because on windows a dll might use a different heap.
		auto node = curl_slist_append(nullptr, str);
		if (node == nullptr) throw std::bad_alloc{};
		if (pos.m_current == nullptr) pos.m_current = m_last_node;
		if (pos.m_current) {
			node->next = pos.m_current->next;
			pos.m_current->next = node;
		} else {
			node->next = nullptr;
			m_first_node = node;
		}
		if (node->next == nullptr) m_last_node = node;
		return slist_iterator{node};
	}

	void slist::remove(size_t index) {
		if (index == 0) {
			auto ptr = m_first_node;
			if (m_last_node == ptr) m_last_node = nullptr;
			if (ptr != nullptr) {
				m_first_node = ptr->next;
				ptr->next = nullptr;
				curl_slist_free_all(ptr);
			}
		} else {
			auto prev = m_first_node;
			if (prev == nullptr) return;
			auto ptr = prev->next;
			for (size_t i = 1; i < index && ptr != nullptr; i++) {
				prev = ptr;
				ptr = ptr->next;
			}
			if (ptr != nullptr) {
				if (ptr == m_last_node) m_last_node = prev;
				prev->next = ptr->next;
				ptr->next = nullptr;
				curl_slist_free_all(ptr);
			}
		}
	}

	void slist::remove(slist_iterator pos) {
		if (pos.m_current == nullptr) pos.m_current = m_last_node;
		if (pos.m_current == nullptr) return;
		if (pos.m_current == m_first_node) {
			m_first_node = pos.m_current->next;
			if (m_first_node == nullptr) m_last_node = nullptr;
		} else {
			auto prev = m_first_node;
			while (prev != nullptr && prev->next != pos.m_current) {
				prev = prev->next;
			}
			if (prev == nullptr) return;
			prev->next = pos.m_current->next;
			if (pos.m_current->next == nullptr) m_last_node = prev;
		}
		pos.m_current->next = nullptr;
		curl_slist_free_all(pos.m_current);
	}

	void slist::clear() {
		curl_slist_free_all(m_first_node);
		m_first_node = nullptr;
		m_last_node = nullptr;
	}

} // namespace asyncpp::curl
