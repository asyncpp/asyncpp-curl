#include <asyncpp/curl/slist.h>
#include <curl/curl.h>
#include <gtest/gtest.h>

using namespace asyncpp::curl;

namespace {
	struct SListTester {
		curl_slist* m_first_node;
		curl_slist* m_last_node;
	};
} // namespace

TEST(ASYNCPP_CURL, SlistCreate) {
	slist list;
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	ASSERT_EQ(t->m_first_node, nullptr);
	ASSERT_EQ(t->m_last_node, nullptr);
}

TEST(ASYNCPP_CURL, SlistCreateCopy) {
	auto n = curl_slist_append(nullptr, "Test");
	slist list{n, slist::ownership_copy};
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	ASSERT_NE(t->m_first_node, nullptr);
	ASSERT_NE(t->m_last_node, nullptr);
	ASSERT_EQ(t->m_first_node, t->m_last_node);
	ASSERT_NE(t->m_first_node, n);
	ASSERT_EQ(t->m_first_node->data, std::string_view{"Test"});
	curl_slist_free_all(n);
}

TEST(ASYNCPP_CURL, SlistCreateTake) {
	auto n = curl_slist_append(nullptr, "Test");
	slist list{n, slist::ownership_take};
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	ASSERT_NE(t->m_first_node, nullptr);
	ASSERT_NE(t->m_last_node, nullptr);
	ASSERT_EQ(t->m_first_node, t->m_last_node);
	ASSERT_EQ(t->m_first_node, n);
	ASSERT_EQ(t->m_first_node->data, std::string_view{"Test"});
}

TEST(ASYNCPP_CURL, SlistAppend) {
	slist list;
	list.append("Test");
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	ASSERT_NE(t->m_first_node, nullptr);
	ASSERT_NE(t->m_last_node, nullptr);
	ASSERT_EQ(t->m_first_node, t->m_last_node);
	ASSERT_EQ(t->m_first_node->next, nullptr);
	ASSERT_NE(t->m_first_node->data, nullptr);
	ASSERT_EQ(t->m_first_node->data, std::string_view{"Test"});
}

TEST(ASYNCPP_CURL, SlistAppendRemove) {
	slist list;
	list.append("Test");
	list.remove(0);
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	ASSERT_EQ(t->m_first_node, nullptr);
	ASSERT_EQ(t->m_last_node, nullptr);
}

TEST(ASYNCPP_CURL, SlistAppendRemoveIterator) {
	slist list;
	list.append("Test");
	list.remove(list.begin());
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	ASSERT_EQ(t->m_first_node, nullptr);
	ASSERT_EQ(t->m_last_node, nullptr);
}

TEST(ASYNCPP_CURL, SlistAppend2Remove) {
	slist list;
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	list.append("Test");
	list.append("Test2");
	ASSERT_NE(t->m_first_node, nullptr);
	ASSERT_NE(t->m_last_node, nullptr);
	ASSERT_NE(t->m_first_node, t->m_last_node);
	ASSERT_NE(t->m_first_node->next, nullptr);
	ASSERT_EQ(t->m_first_node->next, t->m_last_node);
	auto n = t->m_last_node;
	list.remove(0);
	ASSERT_EQ(n, t->m_first_node);
	ASSERT_EQ(n, t->m_last_node);
	ASSERT_EQ(t->m_last_node->next, nullptr);
	list.remove(0);
	ASSERT_EQ(t->m_first_node, nullptr);
	ASSERT_EQ(t->m_last_node, nullptr);
}

TEST(ASYNCPP_CURL, SlistAppend2RemoveLast) {
	slist list;
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	list.append("Test");
	list.append("Test2");
	ASSERT_NE(t->m_first_node, nullptr);
	ASSERT_NE(t->m_last_node, nullptr);
	ASSERT_NE(t->m_first_node, t->m_last_node);
	ASSERT_NE(t->m_first_node->next, nullptr);
	ASSERT_EQ(t->m_first_node->next, t->m_last_node);
	auto n = t->m_first_node;
	list.remove(1);
	ASSERT_EQ(n, t->m_first_node);
	ASSERT_EQ(n, t->m_last_node);
	ASSERT_EQ(t->m_last_node->next, nullptr);
	list.remove(0);
	ASSERT_EQ(t->m_first_node, nullptr);
	ASSERT_EQ(t->m_last_node, nullptr);
}

TEST(ASYNCPP_CURL, SlistAppend2RemoveIterator) {
	slist list;
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	list.append("Test");
	list.append("Test2");
	ASSERT_NE(t->m_first_node, nullptr);
	ASSERT_NE(t->m_last_node, nullptr);
	ASSERT_NE(t->m_first_node, t->m_last_node);
	ASSERT_NE(t->m_first_node->next, nullptr);
	ASSERT_EQ(t->m_first_node->next, t->m_last_node);
	auto n = t->m_last_node;
	list.remove(list.begin());
	ASSERT_EQ(n, t->m_first_node);
	ASSERT_EQ(n, t->m_last_node);
	ASSERT_EQ(t->m_last_node->next, nullptr);
	list.remove(list.begin());
	ASSERT_EQ(t->m_first_node, nullptr);
	ASSERT_EQ(t->m_last_node, nullptr);
}

TEST(ASYNCPP_CURL, SlistAppend2RemoveLastIterator) {
	slist list;
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	list.append("Test");
	list.append("Test2");
	ASSERT_NE(t->m_first_node, nullptr);
	ASSERT_NE(t->m_last_node, nullptr);
	ASSERT_NE(t->m_first_node, t->m_last_node);
	ASSERT_NE(t->m_first_node->next, nullptr);
	ASSERT_EQ(t->m_first_node->next, t->m_last_node);
	auto n = t->m_first_node;
	list.remove(list.end());
	ASSERT_EQ(n, t->m_first_node);
	ASSERT_EQ(n, t->m_last_node);
	ASSERT_EQ(t->m_last_node->next, nullptr);
	list.remove(list.begin());
	ASSERT_EQ(t->m_first_node, nullptr);
	ASSERT_EQ(t->m_last_node, nullptr);
}

TEST(ASYNCPP_CURL, SlistClear) {
	slist list;
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	list.append("Test");
	list.append("Test2");
	ASSERT_NE(t->m_first_node, nullptr);
	ASSERT_NE(t->m_last_node, nullptr);
	list.clear();
	ASSERT_EQ(t->m_first_node, nullptr);
	ASSERT_EQ(t->m_last_node, nullptr);
}

TEST(ASYNCPP_CURL, SlistEmpty) {
	slist list;
	ASSERT_TRUE(list.empty());
	list.append("Test");
	ASSERT_FALSE(list.empty());
}

TEST(ASYNCPP_CURL, SlistInsertFront) {
	slist list;
	list.insert(0, "Test");
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	ASSERT_NE(t->m_first_node, nullptr);
	ASSERT_NE(t->m_last_node, nullptr);
	ASSERT_EQ(t->m_first_node, t->m_last_node);
	ASSERT_EQ(t->m_first_node->next, nullptr);
	ASSERT_NE(t->m_first_node->data, nullptr);
	ASSERT_EQ(t->m_first_node->data, std::string_view{"Test"});
	list.insert(0, "Test2");
	ASSERT_NE(t->m_first_node, t->m_last_node);
	ASSERT_NE(t->m_first_node->next, nullptr);
	ASSERT_NE(t->m_first_node->data, nullptr);
	ASSERT_EQ(t->m_first_node->data, std::string_view{"Test2"});
}

TEST(ASYNCPP_CURL, SlistInsertMiddle) {
	slist list;
	list.append("Test");
	list.append("Test2");
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	auto first = t->m_first_node;
	auto last = t->m_last_node;
	list.insert(1, "Test3");
	ASSERT_EQ(t->m_first_node, first);
	ASSERT_EQ(t->m_last_node, last);
	ASSERT_NE(t->m_first_node->next, last);
	ASSERT_NE(t->m_first_node->next, nullptr);
	ASSERT_EQ(t->m_first_node->next->next, last);
}

TEST(ASYNCPP_CURL, SlistInsertEnd) {
	slist list;
	list.append("Test");
	list.append("Test2");
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	auto first = t->m_first_node;
	auto last = t->m_last_node;
	list.insert(2, "Test3");
	ASSERT_EQ(t->m_first_node, first);
	ASSERT_NE(t->m_last_node, last);
	ASSERT_EQ(t->m_first_node->next, last);
	ASSERT_NE(t->m_first_node->next->next, last);
	ASSERT_EQ(t->m_first_node->next->next, t->m_last_node);
	ASSERT_EQ(t->m_first_node->next->next->next, nullptr);
}

TEST(ASYNCPP_CURL, SlistInsertMiddleIterator) {
	slist list;
	list.append("Test");
	list.append("Test2");
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	auto first = t->m_first_node;
	auto last = t->m_last_node;
	list.insert_after(list.begin(), "Test3");
	ASSERT_EQ(t->m_first_node, first);
	ASSERT_EQ(t->m_last_node, last);
	ASSERT_NE(t->m_first_node->next, last);
	ASSERT_NE(t->m_first_node->next, nullptr);
	ASSERT_EQ(t->m_first_node->next->next, last);
}

TEST(ASYNCPP_CURL, SlistInsertEndIterator) {
	slist list;
	list.append("Test");
	list.append("Test2");
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	auto first = t->m_first_node;
	auto last = t->m_last_node;
	list.insert_after(++(list.begin()), "Test3");
	ASSERT_EQ(t->m_first_node, first);
	ASSERT_NE(t->m_last_node, last);
	ASSERT_EQ(t->m_first_node->next, last);
	ASSERT_NE(t->m_first_node->next->next, last);
	ASSERT_EQ(t->m_first_node->next->next, t->m_last_node);
	ASSERT_EQ(t->m_first_node->next->next->next, nullptr);
}

TEST(ASYNCPP_CURL, SlistInsertEndIterator2) {
	slist list;
	list.append("Test");
	list.append("Test2");
	SListTester* t = reinterpret_cast<SListTester*>(&list);
	auto first = t->m_first_node;
	auto last = t->m_last_node;
	list.insert_after(list.end(), "Test3");
	ASSERT_EQ(t->m_first_node, first);
	ASSERT_NE(t->m_last_node, last);
	ASSERT_EQ(t->m_first_node->next, last);
	ASSERT_NE(t->m_first_node->next->next, last);
	ASSERT_EQ(t->m_first_node->next->next, t->m_last_node);
	ASSERT_EQ(t->m_first_node->next->next->next, nullptr);
}
