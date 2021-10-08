#include <curl/curl.h>
#include <asyncpp/curl/slist.h>
#include <new>
#include <utility>

namespace asyncpp::curl {
	slist_iterator::reference slist_iterator::operator*() const noexcept { return m_current->data; }

	slist_iterator::pointer slist_iterator::operator->() const noexcept { return &m_current->data; }

	slist_iterator& slist_iterator::operator++() noexcept {
		if (m_current) m_current = m_current->next;
		return *this;
	}

	slist_iterator slist_iterator::operator++(int) noexcept {
		slist_iterator tmp = *this;
		if (m_current) m_current = m_current->next;
		return tmp;
	}

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

	slist::slist(curl_slist* raw, ownership_take_tag)
        : m_first_node{raw}
    {
        auto ptr = m_first_node;
        while(ptr) {
            if(!ptr->next) break;
            ptr = ptr->next;
        }
        m_last_node = ptr;
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

	slist::slist(slist&& other) : m_first_node{other.m_first_node}, m_last_node{other.m_last_node} {
		other.m_first_node = nullptr;
		other.m_last_node = nullptr;
	}

	slist& slist::operator=(slist&& other) {
		m_first_node = std::exchange(other.m_first_node, m_first_node);
		m_last_node = std::exchange(other.m_last_node, m_last_node);
		return *this;
	}

	slist_iterator slist::append(const char* str) {
		auto n = curl_slist_append(nullptr, str);
		if (!n) throw std::bad_alloc{};
		if (!m_first_node) m_first_node = n;
		if(m_last_node) m_last_node->next = n;
        m_last_node = n;
		return slist_iterator{n};
	}

	slist_iterator slist::prepend(const char* str) {
		auto n = curl_slist_append(nullptr, str);
		if (!n) throw std::bad_alloc{};
		n->next = m_first_node;
		m_first_node = n;
		if (!m_last_node) m_last_node = n;
		return slist_iterator{n};
	}

	slist_iterator slist::insert(size_t index, const char* str) {
		if (index == 0) {
			return prepend(str);
		} else {
			auto prev = m_first_node;
			if (!prev) return slist_iterator{nullptr};
			auto ptr = prev->next;
			for (size_t i = 1; i < index && ptr; i++) {
				prev = ptr;
				ptr = ptr->next;
			}
			if (prev) {
				auto n = curl_slist_append(nullptr, str);
				if (!n) throw std::bad_alloc{};
				n->next = ptr;
				prev->next = n;
                if(prev == m_last_node) m_last_node = n;
				return slist_iterator{n};
			}
			return slist_iterator{nullptr};
		}
	}

	slist_iterator slist::insert_after(slist_iterator it, const char* str) {
		if (it.m_current) {
            auto n = curl_slist_append(nullptr, str);
		    if (!n) throw std::bad_alloc{};
			n->next = it.m_current->next;
			it.m_current->next = n;
			if (n->next == nullptr) m_last_node = n;
			return slist_iterator{n};
		} else {
			return append(str);
		}
	}

	void slist::remove(size_t index) {
		if (index == 0) {
			auto ptr = m_first_node;
            if(m_last_node == ptr) m_last_node = nullptr;
			if (ptr) {
				m_first_node = ptr->next;
				ptr->next = nullptr;
				curl_slist_free_all(ptr);
			}
		} else {
			auto prev = m_first_node;
			if (!prev) return;
			auto ptr = prev->next;
			for (size_t i = 1; i < index && ptr; i++) {
				prev = ptr;
				ptr = ptr->next;
			}
			if (ptr) {
                if(ptr == m_last_node) m_last_node = prev;
				prev->next = ptr->next;
				ptr->next = nullptr;
				curl_slist_free_all(ptr);
			}
		}
	}

    void slist::remove(slist_iterator it) {
        if(!it.m_current) {
            it.m_current = m_last_node;
        }
        if(it.m_current == nullptr) return;
        if(it.m_current == m_first_node) {
            m_first_node = it.m_current->next;
            if(m_first_node == nullptr)
                m_last_node = nullptr;
        } else {
            auto prev = m_first_node;
			while(prev && prev->next != it.m_current) {
                prev = prev->next;
            }
            if(!prev) return;
            prev->next = it.m_current->next;
            if(it.m_current->next == nullptr)
                m_last_node = prev;
        }
        it.m_current->next = nullptr;
        curl_slist_free_all(it.m_current);
    }

	void slist::clear() {
		curl_slist_free_all(m_first_node);
		m_first_node = nullptr;
		m_last_node = nullptr;
	}

} // namespace thalhammer::curlpp