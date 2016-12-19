//
// IResearch search engine 
// 
// Copyright � 2016 by EMC Corporation, All Rights Reserved
// 
// This software contains the intellectual property of EMC Corporation or is licensed to
// EMC Corporation from third parties. Use of this software and the intellectual property
// contained therein is expressly limited to the terms and conditions of the License
// Agreement under which it is provided by or on behalf of EMC.
// 

#include "tests_shared.hpp"
#include "filter_test_case_base.hpp"
#include "formats/formats_10.hpp" 
#include "store/memory_directory.hpp"
#include "filter_test_case_base.hpp"
#include "analysis/token_attributes.hpp"
#include "search/same_position_filter.hpp"
#include "search/term_filter.hpp" 

namespace ir = iresearch;

namespace tests {

class same_position_filter_test_case : public filter_test_case_base {
 protected:
  void sub_objects_unordered() {
    // add segment
    tests::json_doc_generator gen(
      resource("same_position.json"),
      [] (tests::document& doc, const std::string& name, const tests::json::json_value& data) {
        typedef templates::text_field<std::string> text_field;
        if (data.quoted) {
          // a || b || c
          doc.insert(std::make_shared<text_field>(name, ir::string_ref(data.value), true), true, false);
        } else {
          // _id
          char* czSuffix;
          auto lValue = strtoll(data.value.c_str(), &czSuffix, 10);

          // 'value' can be interpreted as a double
          if (!czSuffix[0]) {
            doc.insert(std::make_shared<tests::long_field>());
            auto& field = (doc.indexed.end() - 1).as<tests::long_field>();
            field.name(name);
            field.value(lValue);
            field.stored(true);
          }
        }

    });
    add_segment(gen);

    // read segment
    ir::index_reader::ptr index = open_reader();
    ASSERT_EQ(1, index->size());
    auto& segment = *index->begin();

    uint64_t expected_id;
    ir::columnstore_reader::value_reader_f visitor = [&expected_id] (ir::data_input& in) {
      const auto actual_value = ir::read_zvlong(in);
      return actual_value == expected_id;
    };
    auto values = segment.values("_id", visitor);
    
    // empty query
    {
      ir::by_same_position q;
      auto prepared = q.prepare(*index);
      auto docs = prepared->execute(segment);
      ASSERT_FALSE(docs->next());
    }

    // { a: 100 } - equal to 'by_term' 
    {
      ir::by_same_position query;
      query.push_back("a", ir::ref_cast<ir::byte_type>(ir::string_ref("100")));

      ir::by_term expected_query;
      expected_query.field("a").term("100");
      
      auto prepared = query.prepare(*index);
      auto expected_prepared = expected_query.prepare(*index);
        
      auto docs = prepared->execute(segment);
      auto expected_docs = prepared->execute(segment);

      ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::invalid(), docs->value());
      while (expected_docs->next()) {
        ASSERT_TRUE(docs->next());
        ASSERT_EQ(expected_docs->value(), docs->value());
      }
      ASSERT_FALSE(docs->next());
      ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::eof(), docs->value());
    }

    // { a: 100, b:30, c:6 }
    {
      ir::by_same_position q;
      q.push_back("a", ir::ref_cast<ir::byte_type>(ir::string_ref("100")));
      q.push_back("b", ir::ref_cast<ir::byte_type>(ir::string_ref("30")));
      q.push_back("c", ir::ref_cast<ir::byte_type>(ir::string_ref("6")));

      auto prepared = q.prepare(*index);

      // next
      {
        auto docs = prepared->execute(segment);
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::invalid(), docs->value());
        expected_id = 6;
        ASSERT_TRUE(docs->next());
        ASSERT_TRUE(values(docs->value()));
        expected_id = 27;
        ASSERT_TRUE(docs->next());
        ASSERT_TRUE(values(docs->value()));
        ASSERT_FALSE(docs->next());
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::eof(), docs->value());
      }

      // seek
      {
        auto docs = prepared->execute(segment);
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::invalid(), docs->value());
        expected_id = 6;
        ASSERT_EQ((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id, docs->seek((ir::type_limits<ir::type_t::doc_id_t>::min)()));
        ASSERT_TRUE(values(docs->value()));
        expected_id = 27;
        ASSERT_EQ((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id, docs->seek(expected_id));
        ASSERT_TRUE(values(docs->value()));
        ASSERT_EQ((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id, docs->seek(8)); // seek backwards
        ASSERT_EQ((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id, docs->seek(27)); // seek to same position
        ASSERT_FALSE(docs->next());
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::eof(), docs->value());
      }
    }
    
    // { c: 8, b:80, a:700 }
    {
      ir::by_same_position q;
      q.push_back("c", ir::ref_cast<ir::byte_type>(ir::string_ref("8")));
      q.push_back("b", ir::ref_cast<ir::byte_type>(ir::string_ref("80")));
      q.push_back("a", ir::ref_cast<ir::byte_type>(ir::string_ref("700")));

      auto prepared = q.prepare(*index);

      // next
      {
        auto docs = prepared->execute(segment);
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::invalid(), docs->value());
        expected_id = 14;
        ASSERT_TRUE(docs->next());
        ASSERT_TRUE(values(docs->value()));
        expected_id = 91;
        ASSERT_TRUE(docs->next());
        ASSERT_TRUE(values(docs->value()));
        ASSERT_FALSE(docs->next());
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::eof(), docs->value());
      }

      // seek
      {
        auto docs = prepared->execute(segment);
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::invalid(), docs->value());
        expected_id = 91;
        ASSERT_EQ((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id, docs->seek(27));
        ASSERT_TRUE(values(docs->value()));
        ASSERT_EQ((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id, docs->seek(8)); // seek backwards
        ASSERT_EQ((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id, docs->seek(27)); // seek to same position
        ASSERT_FALSE(docs->next());
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::eof(), docs->value());
      }
    }
    
    // { a: 700, b:*, c: 7 }
    {
      ir::by_same_position q;
      q.push_back("a", ir::ref_cast<ir::byte_type>(ir::string_ref("700")));
      q.push_back("c", ir::ref_cast<ir::byte_type>(ir::string_ref("7")));

      auto prepared = q.prepare(*index);
      
      // next
      {
        auto docs = prepared->execute(segment);
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::invalid(), docs->value());
        ASSERT_TRUE(docs->next());
        expected_id = 1;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 6;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 11;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 17;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 18;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 23;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 24;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 28;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 38;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 51;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 66;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 79;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_TRUE(docs->next());
        expected_id = 89;
        ASSERT_TRUE(values(docs->value()));
        ASSERT_FALSE(docs->next());
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::eof(), docs->value());
      }
      
      // seek + next
      {
        auto docs = prepared->execute(segment);
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::invalid(), docs->value());
        ASSERT_TRUE(docs->next());
        expected_id = 1;
        ASSERT_TRUE(values(docs->value()));
        expected_id = 28;
        ASSERT_EQ((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id, docs->seek((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id));
        ASSERT_TRUE(values(docs->value()));
        expected_id = 38;
        ASSERT_TRUE(docs->next());
        ASSERT_TRUE(values(docs->value()));
        expected_id = 51;
        ASSERT_EQ((ir::type_limits<ir::type_t::doc_id_t>::min)() + expected_id, docs->seek(45));
        ASSERT_TRUE(values(docs->value()));
        expected_id = 66;
        ASSERT_TRUE(docs->next());
        ASSERT_TRUE(values(docs->value()));
        expected_id = 79;
        ASSERT_TRUE(docs->next());
        ASSERT_TRUE(values(docs->value()));
        expected_id = 89;
        ASSERT_TRUE(docs->next());
        ASSERT_TRUE(values(docs->value()));
        ASSERT_FALSE(docs->next());
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::eof(), docs->value());
      }

      // seek to the end
      {
        auto docs = prepared->execute(segment);
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::invalid(), docs->value());
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::eof(), docs->seek(ir::type_limits<ir::type_t::doc_id_t>::eof()));
        ASSERT_FALSE(docs->next());
        ASSERT_EQ(ir::type_limits<ir::type_t::doc_id_t>::eof(), docs->value());
      }
    }
  }
}; // same_position_filter_test_case 

} // tests

// ----------------------------------------------------------------------------
// --SECTION--                                      by_same_position base tests 
// ----------------------------------------------------------------------------

TEST(by_same_position_test, ctor) {
  ir::by_same_position q;
  ASSERT_EQ(ir::by_same_position::type(), q.type());
  ASSERT_TRUE(q.empty());
  ASSERT_EQ(0, q.size());
  ASSERT_EQ(q.begin(), q.end());
  ASSERT_EQ(ir::boost::no_boost(), q.boost());

  auto& features = ir::by_same_position::features();
  ASSERT_EQ(2, features.size());
  ASSERT_TRUE(features.check<ir::frequency>());
  ASSERT_TRUE(features.check<ir::position>());
}

TEST(by_same_position_test, push_back_insert_clear) {
  ir::by_same_position q;

  // push_back 
  {
    q.push_back("speed", ir::ref_cast<ir::byte_type>(ir::string_ref("quick")));
    const std::string color = "color";
    q.push_back(color, ir::ref_cast<ir::byte_type>(ir::string_ref("brown")));
    const ir::bstring fox = ir::ref_cast<ir::byte_type>(ir::string_ref("fox"));
    q.push_back("name", fox);
    const std::string name = "name";
    const ir::bstring squirrel = ir::ref_cast<ir::byte_type>(ir::string_ref("squirrel"));
    q.push_back(name, squirrel);
    ASSERT_FALSE(q.empty());
    ASSERT_EQ(4, q.size());

    // check elements via iterators 
    {
      auto it = q.begin();
      ASSERT_NE(q.end(), it);
      ASSERT_EQ("speed", it->first);
      ASSERT_EQ(ir::ref_cast<ir::byte_type>(ir::string_ref("quick")), it->second);

      ++it;
      ASSERT_NE(q.end(), it);
      ASSERT_EQ("color", it->first);
      ASSERT_EQ(ir::ref_cast<ir::byte_type>(ir::string_ref("brown")), it->second);

      ++it;
      ASSERT_NE(q.end(), it);
      ASSERT_EQ("name", it->first);
      ASSERT_EQ(ir::ref_cast<ir::byte_type>(ir::string_ref("fox")), it->second);

      ++it;
      ASSERT_NE(q.end(), it);
      ASSERT_EQ("name", it->first);
      ASSERT_EQ(ir::ref_cast<ir::byte_type>(ir::string_ref("squirrel")), it->second);

      ++it;
      ASSERT_EQ(q.end(), it);
    }
  }

  q.clear();
  ASSERT_TRUE(q.empty());
  ASSERT_EQ(0, q.size());
  ASSERT_EQ(q.begin(), q.end());
}

TEST(by_same_position_test, boost) {
  // no boost
  {
    // no branches 
    {
      ir::by_same_position q;

      auto prepared = q.prepare(tests::empty_index_reader::instance());
      ASSERT_EQ(ir::boost::no_boost(), ir::boost::extract(prepared->attributes()));
    }

    // single term
    {
      ir::by_same_position q;
      q.push_back("field", ir::ref_cast<ir::byte_type>(ir::string_ref("quick")));

      auto prepared = q.prepare(tests::empty_index_reader::instance());
      ASSERT_EQ(ir::boost::no_boost(), ir::boost::extract(prepared->attributes()));
    }

    // multiple terms
    {
      ir::by_same_position q;
      q.push_back("field", ir::ref_cast<ir::byte_type>(ir::string_ref("quick")));
      q.push_back("field", ir::ref_cast<ir::byte_type>(ir::string_ref("brown")));

      auto prepared = q.prepare(tests::empty_index_reader::instance());
      ASSERT_EQ(ir::boost::no_boost(), ir::boost::extract(prepared->attributes()));
    }
  }

  // with boost
  {
    iresearch::boost::boost_t boost = 1.5f;
    
    // no terms, return empty query
    {
      ir::by_same_position q;
      q.boost(boost);

      auto prepared = q.prepare(tests::empty_index_reader::instance());
      ASSERT_EQ(ir::boost::no_boost(), ir::boost::extract(prepared->attributes()));
    }

    // single term
    {
      ir::by_same_position q;
      q.push_back("field", ir::ref_cast<ir::byte_type>(ir::string_ref("quick")));
      q.boost(boost);

      auto prepared = q.prepare(tests::empty_index_reader::instance());
      ASSERT_EQ(boost, ir::boost::extract(prepared->attributes()));
    }
    
    // single multiple terms 
    {
      ir::by_same_position q;
      q.push_back("field", ir::ref_cast<ir::byte_type>(ir::string_ref("quick")));
      q.push_back("field", ir::ref_cast<ir::byte_type>(ir::string_ref("brown")));
      q.boost(boost);

      auto prepared = q.prepare(tests::empty_index_reader::instance());
      ASSERT_EQ(boost, ir::boost::extract(prepared->attributes()));
    }
  }
}
TEST(by_same_position_test, equal) {
  ASSERT_EQ(ir::by_same_position(), ir::by_same_position());

  {
    ir::by_same_position q0;
    q0.push_back("speed", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("quick")));
    q0.push_back("color", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("brown")));
    
    ir::by_same_position q1;
    q1.push_back("speed", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("quick")));
    q1.push_back("color", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("brown")));
    ASSERT_EQ(q0, q1);
  }

  {
    ir::by_same_position q0;
    q0.push_back("speed", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("quick")));
    q0.push_back("color", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("brown")));
    q0.push_back("name", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("fox")));
    
    ir::by_same_position q1;
    q1.push_back("speed", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("quick")));
    q1.push_back("color", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("brown")));
    q1.push_back("name", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("squirrel")));
    ASSERT_NE(q0, q1);
  }
  
  {
    ir::by_same_position q0;
    q0.push_back("Speed", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("quick")));
    q0.push_back("color", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("brown")));
    q0.push_back("name", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("fox")));
    
    ir::by_same_position q1;
    q1.push_back("speed", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("quick")));
    q1.push_back("color", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("brown")));
    q1.push_back("name", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("fox")));
    ASSERT_NE(q0, q1);
  }
  
  {
    ir::by_same_position q0;
    q0.push_back("speed", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("quick")));
    q0.push_back("color", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("brown")));
    
    ir::by_same_position q1;
    q1.push_back("speed", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("quick")));
    q1.push_back("color", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("brown")));
    q1.push_back("name", iresearch::ref_cast<iresearch::byte_type>(iresearch::string_ref("fox")));
    ASSERT_NE(q0, q1);
  }
}

// ----------------------------------------------------------------------------
// --SECTION--                           memory_directory + iresearch_format_10
// ----------------------------------------------------------------------------

class memory_same_position_filter_test_case : public tests::same_position_filter_test_case {
protected:
  virtual ir::directory* get_directory() override {
    return new ir::memory_directory();
  }

  virtual ir::format::ptr get_codec() override {
    static ir::version10::format FORMAT;
    return ir::format::ptr(&FORMAT, [](ir::format*)->void{});
  }
};

TEST_F(memory_same_position_filter_test_case, by_same_position) {
  sub_objects_unordered();
}

// ----------------------------------------------------------------------------
// --SECTION--                               fs_directory + iresearch_format_10
// ----------------------------------------------------------------------------

class fs_phrase_filter_test_case : public tests::same_position_filter_test_case {
protected:
  virtual ir::directory* get_directory() override {
    return new ir::memory_directory();
  }

  virtual ir::format::ptr get_codec() override {
    static ir::version10::format FORMAT;
    return ir::format::ptr(&FORMAT, [](ir::format*)->void{});
  }
};

TEST_F(fs_phrase_filter_test_case, by_same_position) {
  sub_objects_unordered();
}