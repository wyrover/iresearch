//
// IResearch search engine 
// 
// Copyright (c) 2016 by EMC Corporation, All Rights Reserved
// 
// This software contains the intellectual property of EMC Corporation or is licensed to
// EMC Corporation from third parties. Use of this software and the intellectual property
// contained therein is expressly limited to the terms and conditions of the License
// Agreement under which it is provided by or on behalf of EMC.
// 

#include "tests_shared.hpp"
#include "index/index_meta.hpp"
#include "store/memory_directory.hpp"
#include "utils/directory_utils.hpp"

namespace tests {
  class directory_utils_tests: public ::testing::Test {

    virtual void SetUp() {
      // Code here will be called immediately after the constructor (right before each test).
    }

    virtual void TearDown() {
      // Code here will be called immediately after each test (right before the destructor).
    }
  };
}

using namespace tests;

// -----------------------------------------------------------------------------
// --SECTION--                                                        test suite
// -----------------------------------------------------------------------------

TEST_F(directory_utils_tests, test_reference) {
  // test clear refs (all free)
  {
    iresearch::memory_directory dir;

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, "abc", true).operator bool());

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));

    attribute->clear();
    ASSERT_TRUE(attribute->refs().empty());
  }

  // test clear refs (in-use)
  {
    iresearch::memory_directory dir;
    auto ref = iresearch::directory_utils::reference(dir, "abc", true);

    ASSERT_TRUE(ref.operator bool());

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));
    ASSERT_THROW(attribute->clear(), iresearch::illegal_state);
  }

  // test add file (no missing)
  {
    iresearch::memory_directory dir;

    ASSERT_FALSE(iresearch::directory_utils::reference(dir, "abc").operator bool());

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_FALSE(attribute.operator bool());
  }

  // test add file (with missing)
  {
    iresearch::memory_directory dir;

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, "abc", true).operator bool());

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));
    ASSERT_FALSE(attribute->refs().contains("def"));
  }

  // test add file
  {
    iresearch::memory_directory dir;
    auto file = dir.create("abc");

    ASSERT_FALSE(!file);
    ASSERT_TRUE(iresearch::directory_utils::reference(dir, "abc").operator bool());

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));
    ASSERT_FALSE(attribute->refs().contains("def"));
  }

  // test add from source
  {
    iresearch::memory_directory dir;
    std::unordered_set<std::string> files;
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return true; };

    files.emplace("abc");
    files.emplace("def");

    auto begin = files.begin();
    auto end = files.end();
    auto source = [&begin, &end]()->const std::string* { 
      if (begin == end) {
        return nullptr;
      }
      return &*(begin++);
    };

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, source, visitor));
    ASSERT_EQ(1, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));
    ASSERT_FALSE(attribute->refs().contains("def"));
  }

  // test add from source (with missing)
  {
    iresearch::memory_directory dir;
    std::unordered_set<std::string> files;
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return true; };

    files.emplace("abc");
    files.emplace("def");

    auto begin = files.begin();
    auto end = files.end();
    auto source = [&begin, &end]() -> const std::string* { 
      if (begin == end) {
        return nullptr;
      }
      
      return &*(begin++);
    };

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, source, visitor, true));
    ASSERT_EQ(2, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));
    ASSERT_TRUE(attribute->refs().contains("def"));
  }

  // test add from source visitor terminate
  {
    iresearch::memory_directory dir;
    std::unordered_set<std::string> files;
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return false; };
    std::unordered_set<std::string> expected_one_of = {
      "abc", "def",
    };

    files.emplace("abc");
    files.emplace("def");

    auto begin = files.begin();
    auto end = files.end();
    auto source = [&begin, &end]()->const std::string*{ 
      if (begin == end) {
        return nullptr;
      }
      return &*(begin++);
    };

    ASSERT_FALSE(iresearch::directory_utils::reference(dir, source, visitor, true));
    ASSERT_EQ(1, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());

    for (auto& file: expected_one_of) {
      if (attribute->refs().contains(file)) {
        expected_one_of.erase(file);
        break;
      }
    }

    ASSERT_EQ(1, expected_one_of.size());

    for (auto& file: expected_one_of) {
      ASSERT_FALSE(attribute->refs().contains(file));
    }
  }

  // test add segment_meta files (empty)
  {
    iresearch::memory_directory dir;
    iresearch::segment_meta meta;
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return true; };

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, meta, visitor, true));
    ASSERT_EQ(0, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_FALSE(attribute.operator bool());
  }

  // test add segment_meta files
  {
    iresearch::memory_directory dir;
    iresearch::segment_meta meta;
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return true; };

    meta.files.emplace("abc");
    meta.files.emplace("def");

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, meta, visitor));
    ASSERT_EQ(1, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));
    ASSERT_FALSE(attribute->refs().contains("def"));
  }

  // test add segment_meta files (with missing)
  {
    iresearch::memory_directory dir;
    iresearch::segment_meta meta;
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return true; };

    meta.files.emplace("abc");
    meta.files.emplace("def");

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, meta, visitor, true));
    ASSERT_EQ(2, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));
    ASSERT_TRUE(attribute->refs().contains("def"));
  }

  // test add segment_meta files visitor terminate
  {
    iresearch::memory_directory dir;
    iresearch::segment_meta meta;
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return false; };
    std::unordered_set<std::string> expected_one_of = {
      "abc",
      "def",
    };

    meta.files.emplace("abc");
    meta.files.emplace("def");

    ASSERT_FALSE(iresearch::directory_utils::reference(dir, meta, visitor, true));
    ASSERT_EQ(1, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());

    for (auto& file: expected_one_of) {
      if (attribute->refs().contains(file)) {
        expected_one_of.erase(file);
        break;
      }
    }

    ASSERT_EQ(1, expected_one_of.size());

    for (auto& file: expected_one_of) {
      ASSERT_FALSE(attribute->refs().contains(file));
    }
  }

  // test add index_meta files (empty)
  {
    iresearch::memory_directory dir;
    iresearch::index_meta meta;
    iresearch::index_meta::index_segments_t segments;
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return true; };

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, meta, visitor));
    ASSERT_EQ(0, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_FALSE(attribute.operator bool());
  }

  // test add index_meta files
  {
    iresearch::memory_directory dir;
    iresearch::index_meta meta;
    iresearch::index_meta::index_segments_t segments(1);
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return true; };

    segments[0].meta.files.emplace("abc");
    segments[0].meta.files.emplace("def");
    meta.add(segments.begin(), segments.end());

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, meta, visitor));
    ASSERT_EQ(1, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));
    ASSERT_FALSE(attribute->refs().contains("def"));
  }

  // test add index_meta files (with missing)
  {
    iresearch::memory_directory dir;
    iresearch::index_meta meta;
    iresearch::index_meta::index_segments_t segments(1);
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return true; };

    segments[0].meta.files.emplace("abc");
    segments[0].meta.files.emplace("def");
    meta.add(segments.begin(), segments.end());

    ASSERT_TRUE(iresearch::directory_utils::reference(dir, meta, visitor, true));
    ASSERT_EQ(3, count); // +1 for segment file

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());
    ASSERT_TRUE(attribute->refs().contains("abc"));
    ASSERT_TRUE(attribute->refs().contains("def"));
  }

  // test add index_meta files visitor terminate
  {
    iresearch::memory_directory dir;
    iresearch::index_meta meta;
    iresearch::index_meta::index_segments_t segments(1);
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);
    size_t count = 0;
    auto visitor = [&count](iresearch::index_file_refs::ref_t&& ref)->bool { ++count; return false; };
    std::unordered_set<std::string> expected_one_of = {
      "abc",
      "def",
      "xyz",
    };

    segments[0].filename = "xyz";
    segments[0].meta.files.emplace("abc");
    segments[0].meta.files.emplace("def");
    meta.add(segments.begin(), segments.end());

    ASSERT_FALSE(iresearch::directory_utils::reference(dir, meta, visitor, true));
    ASSERT_EQ(1, count);

    auto& attribute = dir.attributes().get<iresearch::index_file_refs>();

    ASSERT_TRUE(attribute.operator bool());
    ASSERT_FALSE(attribute->refs().empty());

    for (auto& file: expected_one_of) {
      if (attribute->refs().contains(file)) {
        expected_one_of.erase(file);
        break;
      }
    }

    ASSERT_EQ(2, expected_one_of.size());

    for (auto& file: expected_one_of) {
      ASSERT_FALSE(attribute->refs().contains(file));
    }
  }
}

TEST_F(directory_utils_tests, test_ref_tracking_dir) {
  // test move constructor
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir1(dir);
    iresearch::ref_tracking_directory track_dir2(std::move(track_dir1));

    ASSERT_EQ(&dir, &(*track_dir2));
  }

  // test dereference and attributes
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir(dir);

    ASSERT_EQ(&dir, &(*track_dir));
    ASSERT_EQ(&(dir.attributes()), &(track_dir.attributes()));
  }

  // test make_lock
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir(dir);
    auto lock1 = dir.make_lock("abc");
    ASSERT_FALSE(!lock1);
    auto lock2 = track_dir.make_lock("abc");
    ASSERT_FALSE(!lock2);

    ASSERT_TRUE(lock1->lock());
    ASSERT_FALSE(lock2->lock());
    ASSERT_TRUE(lock1->unlock());
    ASSERT_TRUE(lock2->lock());
  }

  // test open
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir(dir);
    auto file1 = dir.create("abc");

    ASSERT_FALSE(!file1);
    file1->write_byte(42);
    file1->flush();

    auto file2 = track_dir.open("abc");

    ASSERT_FALSE(!file2);
    ASSERT_TRUE(track_dir.sync("abc")); // does nothing in memory_directory, but adds line coverage
    ASSERT_EQ(1, file2->length());
    ASSERT_TRUE(track_dir.rename("abc", "def"));
    file1.reset(); // release before remove
    file2.reset(); // release before remove
    ASSERT_FALSE(track_dir.remove("abc"));
    bool exists;
    ASSERT_TRUE(dir.exists(exists, "abc") && !exists);
    ASSERT_TRUE(dir.exists(exists, "def") && exists);
    ASSERT_TRUE(track_dir.remove("def"));
    ASSERT_TRUE(dir.exists(exists, "def") && !exists);
  }

  // test close (direct)
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir(dir);
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);

    bool exists;
    ASSERT_TRUE(dir.exists(exists, "abc") && exists);
    ASSERT_TRUE(track_dir.exists(exists, "abc") && exists);
    file->write_byte(42);
    file->flush();
    uint64_t length;
    ASSERT_TRUE(dir.length(length, "abc") && length == 1);
    ASSERT_TRUE(track_dir.length(length, "abc") && length == 1);

    std::vector<std::string> files;
    auto list_files = [&files] (std::string& name) {
      files.emplace_back(std::move(name));
      return true;
    };
    ASSERT_TRUE(track_dir.visit(list_files));
    ASSERT_EQ(1, files.size());
    ASSERT_EQ("abc", *(files.begin()));

    file.reset(); // release before dir.close()
    dir.close(); // close via memory_directory (clears file list)
    ASSERT_TRUE(dir.exists(exists, "abc") && !exists);
    ASSERT_TRUE(track_dir.exists(exists, "abc") && !exists);
  }

  // test close (indirect)
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir(dir);
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);

    bool exists;
    ASSERT_TRUE(dir.exists(exists, "abc") && exists);
    ASSERT_TRUE(track_dir.exists(exists, "abc") && exists);
    file->write_byte(42);
    file->flush();
    uint64_t length;
    ASSERT_TRUE(dir.length(length, "abc") && length == 1);
    ASSERT_TRUE(track_dir.length(length, "abc") && length == 1);

    std::vector<std::string> files;
    auto list_files = [&files] (std::string& name) {
      files.emplace_back(std::move(name));
      return true;
    };
    ASSERT_TRUE(track_dir.visit(list_files));
    ASSERT_EQ(1, files.size());
    ASSERT_EQ("abc", *(files.begin()));

    file.reset(); // release before dir.close()
    track_dir.close(); // close via tracking_directory (clears file list)
    ASSERT_TRUE(dir.exists(exists, "abc") && !exists);
    ASSERT_TRUE(track_dir.exists(exists, "abc") && !exists);
  }

  // test visit refs visitor complete
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir(dir);
    auto file1 = track_dir.create("abc");
    ASSERT_FALSE(!file1);
    auto file2 = track_dir.create("def");
    ASSERT_FALSE(!file2);
    size_t count = 0;
    auto visitor = [&count](const iresearch::index_file_refs::ref_t& ref)->bool { ++count; return true; };

    ASSERT_TRUE(track_dir.visit_refs(visitor));
    ASSERT_EQ(2, count);
  }

  // test visit refs visitor (no-track-open)
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir(dir);
    auto file1 = dir.create("abc");
    ASSERT_FALSE(!file1);
    auto file2 = track_dir.open("abc");
    ASSERT_FALSE(!file2);
    size_t count = 0;
    auto visitor = [&count](const iresearch::index_file_refs::ref_t& ref)->bool { ++count; return true; };

    ASSERT_TRUE(track_dir.visit_refs(visitor));
    ASSERT_EQ(0, count);
  }

  // test visit refs visitor (track-open)
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir(dir, true);
    auto file1 = dir.create("abc");
    ASSERT_FALSE(!file1);
    auto file2 = track_dir.open("abc");
    ASSERT_FALSE(!file2);
    size_t count = 0;
    auto visitor = [&count](const iresearch::index_file_refs::ref_t& ref)->bool { ++count; return true; };

    ASSERT_TRUE(track_dir.visit_refs(visitor));
    ASSERT_EQ(1, count);
  }

  // test visit refs visitor terminate
  {
    iresearch::memory_directory dir;
    iresearch::ref_tracking_directory track_dir(dir);
    auto file1 = track_dir.create("abc");
    ASSERT_FALSE(!file1);
    auto file2 = track_dir.create("def");
    ASSERT_FALSE(!file2);
    size_t count = 0;
    auto visitor = [&count](const iresearch::index_file_refs::ref_t& ref)->bool { ++count; return false; };

    ASSERT_FALSE(track_dir.visit_refs(visitor));
    ASSERT_EQ(1, count);
  }

  // ...........................................................................
  // errors during file operations
  // ...........................................................................

  struct error_directory: public iresearch::directory {
    iresearch::attributes attrs;
    virtual iresearch::attributes& attributes() NOEXCEPT override { return attrs; }
    virtual void close() NOEXCEPT override {}
    virtual iresearch::index_output::ptr create(const std::string&) NOEXCEPT override { return nullptr; }
    virtual bool exists(bool& result, const std::string&) const NOEXCEPT override { return false; }
    virtual bool length(uint64_t& result, const std::string&) const NOEXCEPT override { return false; }
    virtual bool visit(const visitor_f&) const override { return false; }
    virtual iresearch::index_lock::ptr make_lock(const std::string&) NOEXCEPT override { return nullptr; }
    virtual bool mtime(std::time_t& result, const std::string& name) const NOEXCEPT override { return false; }
    virtual iresearch::index_input::ptr open(const std::string&) const NOEXCEPT override { return nullptr; }
    virtual bool remove(const std::string&) NOEXCEPT override { return false; }
    virtual bool rename(const std::string&, const std::string&) NOEXCEPT override { return false; }
    virtual bool sync(const std::string& name) NOEXCEPT override { return false; }
  } error_dir;

  // test create failure
  {
    error_dir.attributes().clear();

    iresearch::ref_tracking_directory track_dir(error_dir);

    ASSERT_FALSE(track_dir.create("abc"));

    std::set<std::string> refs;
    auto visitor = [&refs](const iresearch::index_file_refs::ref_t& ref)->bool { refs.insert(*ref); return true; };

    ASSERT_TRUE(track_dir.visit_refs(visitor));
    ASSERT_TRUE(refs.empty());
  }

  // test open failure
  {
    error_dir.attributes().clear();

    iresearch::ref_tracking_directory track_dir(error_dir, true);

    ASSERT_FALSE(track_dir.open("abc"));

    std::set<std::string> refs;
    auto visitor = [&refs](const iresearch::index_file_refs::ref_t& ref)->bool { refs.insert(*ref); return true; };

    ASSERT_TRUE(track_dir.visit_refs(visitor));
    ASSERT_TRUE(refs.empty());
  }
}

TEST_F(directory_utils_tests, test_tracking_dir) {
  // test dereference and attributes
  {
    iresearch::memory_directory dir;
    iresearch::tracking_directory track_dir(dir);

    ASSERT_EQ(&dir, &(*track_dir));
    ASSERT_EQ(&(dir.attributes()), &(track_dir.attributes()));
  }

  // test make_lock
  {
    iresearch::memory_directory dir;
    iresearch::tracking_directory track_dir(dir);
    auto lock1 = dir.make_lock("abc");
    ASSERT_FALSE(!lock1);
    auto lock2 = track_dir.make_lock("abc");
    ASSERT_FALSE(!lock2);

    ASSERT_TRUE(lock1->lock());
    ASSERT_FALSE(lock2->lock());
    ASSERT_TRUE(lock1->unlock());
    ASSERT_TRUE(lock2->lock());
  }

  // test open
  {
    iresearch::memory_directory dir;
    iresearch::tracking_directory track_dir(dir);
    auto file1 = dir.create("abc");
    ASSERT_FALSE(!file1);

    file1->write_byte(42);
    file1->flush();

    auto file2 = track_dir.open("abc");
    ASSERT_FALSE(!file2);

    ASSERT_TRUE(track_dir.sync("abc")); // does nothing in memory_directory, but adds line coverage
    ASSERT_EQ(1, file2->length());
    ASSERT_TRUE(track_dir.rename("abc", "def"));
    file1.reset(); // release before remove
    file2.reset(); // release before remove
    ASSERT_FALSE(track_dir.remove("abc"));
    bool exists;
    ASSERT_TRUE(dir.exists(exists, "abc") && !exists);
    ASSERT_TRUE(dir.exists(exists, "def") && exists);
    ASSERT_TRUE(track_dir.remove("def"));
    ASSERT_TRUE(dir.exists(exists, "def") && !exists);
  }

  // test open (no-track-open)
  {
    iresearch::memory_directory dir;
    iresearch::tracking_directory track_dir(dir);
    auto file1 = dir.create("abc");
    ASSERT_FALSE(!file1);
    auto file2 = track_dir.open("abc");
    ASSERT_FALSE(!file2);
    iresearch::tracking_directory::file_set files;

    ASSERT_TRUE(track_dir.swap_tracked(files));
    ASSERT_EQ(0, files.size());
  }

  // test open (track-open)
  {
    iresearch::memory_directory dir;
    iresearch::tracking_directory track_dir(dir, true);
    auto file1 = dir.create("abc");
    ASSERT_FALSE(!file1);
    auto file2 = track_dir.open("abc");
    ASSERT_FALSE(!file2);
    iresearch::tracking_directory::file_set files;

    ASSERT_TRUE(track_dir.swap_tracked(files));
    ASSERT_EQ(1, files.size());
  }

  // test close (direct)
  {
    iresearch::memory_directory dir;
    iresearch::tracking_directory track_dir(dir);
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);

    bool exists;
    ASSERT_TRUE(dir.exists(exists, "abc") && exists);
    ASSERT_TRUE(track_dir.exists(exists, "abc") && exists);
    file->write_byte(42);
    file->flush();
    uint64_t length;
    ASSERT_TRUE(dir.length(length, "abc") && length == 1);
    ASSERT_TRUE(track_dir.length(length, "abc") && length == 1);

    std::vector<std::string> files;
    auto list_files = [&files] (std::string& name) {
      files.emplace_back(std::move(name));
      return true;
    };
    ASSERT_TRUE(track_dir.visit(list_files));
    ASSERT_EQ(1, files.size());
    ASSERT_EQ("abc", *(files.begin()));

    file.reset(); // release before dir.close()
    dir.close(); // close via memory_directory (clears file list)
    ASSERT_TRUE(dir.exists(exists, "abc") && !exists);
    ASSERT_TRUE(track_dir.exists(exists, "abc") && !exists);
  }

  // test close (indirect)
  {
    iresearch::memory_directory dir;
    iresearch::tracking_directory track_dir(dir);
    auto file = dir.create("abc");
    ASSERT_FALSE(!file);

    bool exists;
    ASSERT_TRUE(dir.exists(exists, "abc") && exists);
    ASSERT_TRUE(track_dir.exists(exists, "abc") && exists);
    file->write_byte(42);
    file->flush();
    uint64_t length;
    ASSERT_TRUE(dir.length(length, "abc") && length == 1);
    ASSERT_TRUE(track_dir.length(length, "abc") && length == 1);

    std::vector<std::string> files;
    auto list_files = [&files] (std::string& name) {
      files.emplace_back(std::move(name));
      return true;
    };
    ASSERT_TRUE(track_dir.visit(list_files));
    ASSERT_EQ(1, files.size());
    ASSERT_EQ("abc", *(files.begin()));

    file.reset(); // release before track_dir.close()
    track_dir.close(); // close via tracking_directory (clears file list)
    ASSERT_TRUE(dir.exists(exists, "abc") && !exists);
    ASSERT_TRUE(track_dir.exists(exists, "abc") && !exists);
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------
