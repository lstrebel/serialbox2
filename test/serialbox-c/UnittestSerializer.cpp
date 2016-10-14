//===-- serialbox-c/UnittestSerializer.cpp ------------------------------------------*- C++ -*-===//
//
//                                    S E R I A L B O X
//
// This file is distributed under terms of BSD license.
// See LICENSE.txt for more information
//
//===------------------------------------------------------------------------------------------===//
//
/// \file
/// This file implements the unittests of the C Interface Serializer.
///
//===------------------------------------------------------------------------------------------===//

#include "Utility/CInterfaceTestBase.h"
#include "Utility/Storage.h"
#include "serialbox-c/FieldMetaInfo.h"
#include "serialbox-c/MetaInfo.h"
#include "serialbox-c/Savepoint.h"
#include "serialbox-c/Serializer.h"
#include <gtest/gtest.h>

namespace {

class CSerializerUtilityTest : public serialbox::unittest::CInterfaceTestBase {};

} // anonymous namespace

TEST_F(CSerializerUtilityTest, Construction) {
  // -----------------------------------------------------------------------------------------------
  // Write
  // -----------------------------------------------------------------------------------------------
  {
    // Open fresh serializer and write meta data to disk
    serialboxSerializer_t* ser =
        serialboxSerializerCreate(Write, directory->path().c_str(), "Field", "Binary");
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    EXPECT_EQ(serialboxSerializerGetMode(ser), Write);
    EXPECT_STREQ(serialboxSerializerGetDirectory(ser), directory->path().c_str());
    EXPECT_STREQ(serialboxSerializerGetPrefix(ser), "Field");

    serialboxSerializerUpdateMetaData(ser);
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    serialboxSerializerDestroy(ser);
  }

  {
    // Directory does not exists (should be created by the Archive)
    serialboxSerializer_t* ser = serialboxSerializerCreate(
        Write, (directory->path() / "dir-is-created-from-write").c_str(), "Field", "Binary");
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    ASSERT_TRUE(boost::filesystem::exists(directory->path() / "dir-is-created-from-write"));

    serialboxSerializerUpdateMetaData(ser);
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    serialboxSerializerDestroy(ser);
  }

  // -----------------------------------------------------------------------------------------------
  // Read
  // -----------------------------------------------------------------------------------------------
  {
    // MetaData.json exists (from Writing part)
    serialboxSerializer_t* ser =
        serialboxSerializerCreate(Read, directory->path().c_str(), "Field", "Binary");
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    serialboxSerializerDestroy(ser);
  }

  {
    // Directory does not exist -> FatalError
    serialboxSerializer_t* ser = serialboxSerializerCreate(
        Read, (directory->path() / "not-a-dir").c_str(), "Field", "Binary");
    ASSERT_TRUE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    serialboxSerializerDestroy(ser);
  }

  {
    // MetaData-prefix.json does not exist -> Exception
    boost::filesystem::remove((directory->path() / "dir-is-created-from-write") /
                              "MetaData-Field.json");

    serialboxSerializer_t* ser = serialboxSerializerCreate(
        Read, (directory->path() / "dir-is-created-from-write").c_str(), "Field", "Binary");
    ASSERT_TRUE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    serialboxSerializerDestroy(ser);
  }

  // -----------------------------------------------------------------------------------------------
  // Append
  // -----------------------------------------------------------------------------------------------
  {
    // Construct from existing (empty) metaData
    serialboxSerializer_t* ser =
        serialboxSerializerCreate(Append, directory->path().c_str(), "Field", "Binary");
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    serialboxSerializerDestroy(ser);
  }

  {
    // Directory does not exists (should be created by the Archive)
    serialboxSerializer_t* ser = serialboxSerializerCreate(
        Append, (directory->path() / "dir-is-created-from-append").c_str(), "Field", "Binary");
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    ASSERT_TRUE(boost::filesystem::exists(directory->path() / "dir-is-created-from-append"));
    serialboxSerializerDestroy(ser);
  }
}

TEST_F(CSerializerUtilityTest, AddMetaInfo) {
  serialboxSerializer_t* ser =
      serialboxSerializerCreate(Write, directory->path().c_str(), "Field", "Binary");

  // Add meta-information
  {
    serialboxMetaInfo_t* metaInfo = serialboxSerializerGetGlobalMetaInfo(ser);
    ASSERT_FALSE(metaInfo->ownsData);
    ASSERT_TRUE(serialboxMetaInfoAddBoolean(metaInfo, "key", true));
    serialboxMetaInfoDestroy(metaInfo);
  }

  // Query meta-information
  {
    serialboxMetaInfo_t* metaInfo = serialboxSerializerGetGlobalMetaInfo(ser);
    ASSERT_FALSE(metaInfo->ownsData);
    ASSERT_TRUE(serialboxMetaInfoHasKey(metaInfo, "key"));
    EXPECT_EQ(serialboxMetaInfoGetBoolean(metaInfo, "key"), true);
    serialboxMetaInfoDestroy(metaInfo);
  }

  serialboxSerializerDestroy(ser);
}

TEST_F(CSerializerUtilityTest, RegisterSavepoints) {
  serialboxSerializer_t* ser =
      serialboxSerializerCreate(Write, directory->path().c_str(), "Field", "Binary");

  // Create Savepoints
  serialboxSavepoint_t* savepoint1 = serialboxSavepointCreate("savepoint1");
  serialboxMetaInfoAddBoolean(serialboxSavepointGetMetaInfo(savepoint1), "key", true);

  serialboxSavepoint_t* savepoint2 = serialboxSavepointCreate("savepoint2");

  // Add Savepoint to Serializer
  ASSERT_TRUE(serialboxSerializerAddSavepoint(ser, savepoint1));
  ASSERT_TRUE(serialboxSerializerAddSavepoint(ser, savepoint2));

  // Query Savepoints
  int numSavepoints = serialboxSerializerGetNumSavepoints(ser);
  ASSERT_EQ(numSavepoints, 2);

  serialboxSavepoint_t** savepoints = serialboxSerializerGetSavepointVector(ser);
  EXPECT_TRUE(serialboxSavepointEqual(savepoints[0], savepoint1));
  EXPECT_TRUE(serialboxSavepointEqual(savepoints[1], savepoint2));

  serialboxSavepointDestroy(savepoint1);
  serialboxSavepointDestroy(savepoint2);
  serialboxSerializerDestroySavepointVector(savepoints, numSavepoints);
  serialboxSerializerDestroy(ser);
}

TEST_F(CSerializerUtilityTest, RegisterFields) {
  serialboxSerializer_t* ser =
      serialboxSerializerCreate(Write, directory->path().c_str(), "Field", "Binary");

  // Create FieldMetaInfo
  int dims[3] = {10, 15, 20};
  serialboxFieldMetaInfo_t* info = serialboxFieldMetaInfoCreate(Float64, dims, 3);

  // Register field
  ASSERT_TRUE(serialboxSerializerAddField(ser, "field", info));
  ASSERT_FALSE(serialboxSerializerAddField(ser, "field", info));

  // Register field (old version)
  ASSERT_TRUE(
      serialboxSerializerAddField2(ser, "field2", Int32, 4, 42, 1, 1, 12, 1, 1, 0, 0, 0, 0, 2, 2));
  ASSERT_FALSE(
      serialboxSerializerAddField2(ser, "field2", Int32, 4, 42, 1, 1, 12, 1, 1, 0, 0, 0, 0, 2, 2));

  // Query fieldnames
  int len;
  char** fieldnames;
  serialboxSerializerGetFieldnames(ser, &fieldnames, &len);

  ASSERT_EQ(len, 2);
  ASSERT_TRUE(std::find_if(fieldnames, fieldnames + len, [](const char* s) {
                return (std::memcmp(s, "field", sizeof("field")) == 0);
              }) != (fieldnames + len));
  ASSERT_TRUE(std::find_if(fieldnames, fieldnames + len, [](const char* s) {
                return (std::memcmp(s, "field2", sizeof("field2")) == 0);
              }) != (fieldnames + len));

  for(int i = 0; i < len; ++i)
    std::free(fieldnames[i]);
  std::free(fieldnames);

  // Get FieldMetaInfo of "field"
  serialboxFieldMetaInfo_t* infoField = serialboxSerializerGetFieldMetaInfo(ser, "field");
  ASSERT_TRUE(serialboxFieldMetaInfoEqual(info, infoField));

  // Get FieldMetaInfo of "field2"

  // TODO...
  
  //  serialboxFieldMetaInfo_t* infoField2 = serialboxSerializerGetFieldMetaInfo(ser, "field2");

  //  int numDimension = serialboxFieldMetaInfoGetNumDimensions(infoField2);
  //  const int* dimension = serialboxFieldMetaInfoGetDimensions(infoField2);
  //  serialboxTypeID type = serialboxFieldMetaInfoGetTypeID(infoField2);
  //  serialboxMetaInfo_t* metaInfo = serialboxFieldMetaInfoGetMetaInfo(infoField2);

  // Get FieldMetaInfo of non-existing field -> NULL
  ASSERT_EQ(NULL, serialboxSerializerGetFieldMetaInfo(ser, "X"));

  serialboxFieldMetaInfoDestroy(info);
  serialboxFieldMetaInfoDestroy(infoField);
  serialboxSerializerDestroy(ser);
}

namespace {

template <class T>
class CSerializerReadWriteTest : public serialbox::unittest::CInterfaceTestBase {};

using TestTypes = testing::Types<double, float, int, std::int64_t>;

} // anonymous namespace

TYPED_TEST_CASE(CSerializerReadWriteTest, TestTypes);

TYPED_TEST(CSerializerReadWriteTest, WriteAndRead) {

  serialboxDisableSerialization();
  EXPECT_FALSE(serialboxSerializationEnabled);

  serialboxEnableSerialization();
  EXPECT_TRUE(serialboxSerializationEnabled);

  // -----------------------------------------------------------------------------------------------
  // Preparation
  // -----------------------------------------------------------------------------------------------
  using Storage = serialbox::unittest::Storage<TypeParam>;

  // Prepare input data
  Storage u_0_input(Storage::RowMajor, {5, 6, 7}, {{2, 2}, {4, 2}, {4, 5}}, Storage::random);
  Storage u_1_input(Storage::RowMajor, {5, 6, 7}, {{2, 2}, {4, 2}, {4, 5}}, Storage::random);

  Storage v_0_input(Storage::ColMajor, {5, 1, 1}, Storage::random);
  Storage v_1_input(Storage::ColMajor, {5, 1, 1}, Storage::random);

  Storage field_6d_input(Storage::RowMajor, {2, 2, 1, 2, 1, 2}, Storage::random);

  // Prepare output
  Storage u_0_output(Storage::RowMajor, {5, 6, 7});
  Storage u_1_output(Storage::RowMajor, {5, 6, 7});

  Storage v_0_output(Storage::RowMajor, {5, 1, 1});
  Storage v_1_output(Storage::RowMajor, {5, 1, 1});

  Storage field_6d_output(Storage::RowMajor, {2, 2, 1, 2, 1, 2});

  // Savepoints
  serialboxSavepoint_t* savepoint1_t_1 = serialboxSavepointCreate("savepoint1");
  serialboxMetaInfoAddBoolean(serialboxSavepointGetMetaInfo(savepoint1_t_1), "time", int(1));

  serialboxSavepoint_t* savepoint1_t_2 = serialboxSavepointCreate("savepoint2");
  serialboxMetaInfoAddBoolean(serialboxSavepointGetMetaInfo(savepoint1_t_2), "time", int(2));

  serialboxSavepoint_t* savepoint_u_1 = serialboxSavepointCreate("savepoint_u_1");
  serialboxSavepoint_t* savepoint_v_1 = serialboxSavepointCreate("savepoint_v_1");
  serialboxSavepoint_t* savepoint_6d = serialboxSavepointCreate("savepoint_6d");

  // -----------------------------------------------------------------------------------------------
  // Writing / Appending
  // -----------------------------------------------------------------------------------------------
  //
  //  Savepoint     | MetaData   | Fields
  //  -------------------------------------
  //  savepoint1    | time: 1    | u_0, v_0
  //  savepoint1    | time: 2    | u_1, v_1
  //  savepoint_u_1 | -          | u_1
  //  savepoint_v_1 | -          | v_1
  //  savepoint_6d  | -          | field_6d
  //
  {
    serialboxSerializer_t* ser_write =
        serialboxSerializerCreate(Write, this->directory->path().c_str(), "Field", "Binary");

    // Register fields
    serialboxTypeID type = static_cast<serialboxTypeID>((int)serialbox::ToTypeID<TypeParam>::value);
    serialboxFieldMetaInfo_t* info_u =
        serialboxFieldMetaInfoCreate(type, u_0_input.dims().data(), u_0_input.dims().size());
    serialboxFieldMetaInfo_t* info_v =
        serialboxFieldMetaInfoCreate(type, v_0_input.dims().data(), v_0_input.dims().size());

    ASSERT_TRUE(serialboxSerializerAddField(ser_write, "u", info_u));
    ASSERT_TRUE(serialboxSerializerAddField(ser_write, "v", info_v));

    // Writing (implicitly register the savepoints)

    // u_0 at savepoint1_t_1
    serialboxSerializerWrite(ser_write, "u", savepoint1_t_1, (void*)u_0_input.originPtr(),
                             u_0_input.strides().data(), u_0_input.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    // v_0 at savepoint1_t_1
    serialboxSerializerWrite(ser_write, "v", savepoint1_t_1, (void*)v_0_input.originPtr(),
                             v_0_input.strides().data(), v_0_input.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    // u_1 at savepoint1_t_2
    serialboxSerializerWrite(ser_write, "u", savepoint1_t_2, (void*)u_1_input.originPtr(),
                             u_1_input.strides().data(), u_1_input.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    // v_1 at savepoint1_t_2
    serialboxSerializerWrite(ser_write, "v", savepoint1_t_2, (void*)v_1_input.originPtr(),
                             v_1_input.strides().data(), v_1_input.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    // u_1 at savepoint_u_1
    serialboxSerializerWrite(ser_write, "u", savepoint_u_1, (void*)u_1_input.originPtr(),
                             u_1_input.strides().data(), u_1_input.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    // v_1 at savepoint_v_1
    serialboxSerializerWrite(ser_write, "v", savepoint_v_1, (void*)v_1_input.originPtr(),
                             v_1_input.strides().data(), v_1_input.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    // Field does not exists -> FatalError
    serialboxSerializerWrite(ser_write, "XXX", savepoint_v_1, (void*)v_1_input.originPtr(),
                             v_1_input.strides().data(), v_1_input.strides().size());
    ASSERT_TRUE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    // Inconsistent number of dimensions and strides -> FatalError
    serialboxSerializerWrite(ser_write, "u", savepoint_v_1, (void*)field_6d_input.originPtr(),
                             field_6d_input.strides().data(), field_6d_input.strides().size());
    ASSERT_TRUE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    serialboxFieldMetaInfoDestroy(info_u);
    serialboxFieldMetaInfoDestroy(info_v);
    serialboxSerializerDestroy(ser_write);
  }

  // Reopen serializer and append a data field
  {
    serialboxSerializer_t* ser_append =
        serialboxSerializerCreate(Append, this->directory->path().c_str(), "Field", "Binary");

    serialboxTypeID type = static_cast<serialboxTypeID>((int)serialbox::ToTypeID<TypeParam>::value);
    serialboxFieldMetaInfo_t* info_field_6d = serialboxFieldMetaInfoCreate(
        type, field_6d_input.dims().data(), field_6d_input.dims().size());
    ASSERT_TRUE(serialboxSerializerAddField(ser_append, "field_6d", info_field_6d));

    // field_6d at savepoint_6d
    serialboxSerializerWrite(ser_append, "field_6d", savepoint_6d,
                             (void*)field_6d_input.originPtr(), field_6d_input.strides().data(),
                             field_6d_input.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();

    serialboxFieldMetaInfoDestroy(info_field_6d);
    serialboxSerializerDestroy(ser_append);
  }

  // -----------------------------------------------------------------------------------------------
  // Reading
  // -----------------------------------------------------------------------------------------------
  {
    serialboxSerializer_t* ser_read =
        serialboxSerializerCreate(Read, this->directory->path().c_str(), "Field", "Binary");

    // Check order of savepoints is correct
    int numSavepoints = serialboxSerializerGetNumSavepoints(ser_read);
    ASSERT_EQ(numSavepoints, 5);

    serialboxSavepoint_t** savepoints = serialboxSerializerGetSavepointVector(ser_read);

    EXPECT_TRUE(serialboxSavepointEqual(savepoints[0], savepoint1_t_1));
    EXPECT_TRUE(serialboxSavepointEqual(savepoints[1], savepoint1_t_2));
    EXPECT_TRUE(serialboxSavepointEqual(savepoints[2], savepoint_u_1));
    EXPECT_TRUE(serialboxSavepointEqual(savepoints[3], savepoint_v_1));
    EXPECT_TRUE(serialboxSavepointEqual(savepoints[4], savepoint_6d));

    serialboxSerializerDestroySavepointVector(savepoints, numSavepoints);

    // Check fields at savepoint
    int len;
    char** fieldnames;
    serialboxSerializerGetFieldnamesAtSavepoint(ser_read, savepoint1_t_1, &fieldnames, &len);

    ASSERT_EQ(len, 2);

    // Check u exists
    EXPECT_TRUE(std::find_if(fieldnames, fieldnames + len, [](const char* s) {
                  return (std::memcmp(s, "u", sizeof("u")) == 0);
                }) != (fieldnames + len));

    // Check v exists
    EXPECT_TRUE(std::find_if(fieldnames, fieldnames + len, [](const char* s) {
                  return (std::memcmp(s, "v", sizeof("v")) == 0);
                }) != (fieldnames + len));

    for(int i = 0; i < len; ++i)
      std::free(fieldnames[i]);
    std::free(fieldnames);

    // Read

    // u_0 at savepoint1_t_1
    serialboxSerializerRead(ser_read, "u", savepoint1_t_1, (void*)u_0_output.originPtr(),
                            u_0_output.strides().data(), u_0_output.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    ASSERT_TRUE(Storage::verify(u_0_output, u_0_input));

    // v_0 at savepoint1_t_1
    serialboxSerializerRead(ser_read, "v", savepoint1_t_1, (void*)v_0_output.originPtr(),
                            v_0_output.strides().data(), v_0_output.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    ASSERT_TRUE(Storage::verify(v_0_output, v_0_input));

    // u_1 at savepoint1_t_2
    serialboxSerializerRead(ser_read, "u", savepoint1_t_2, (void*)u_1_output.originPtr(),
                            u_1_output.strides().data(), u_1_output.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    ASSERT_TRUE(Storage::verify(u_1_output, u_1_input));

    // v_1 at savepoint1_t_2
    serialboxSerializerRead(ser_read, "v", savepoint1_t_2, (void*)v_1_output.originPtr(),
                            v_1_output.strides().data(), v_1_output.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    ASSERT_TRUE(Storage::verify(v_1_output, v_1_input));

    // u_1 at savepoint_u_1
    serialboxSerializerRead(ser_read, "u", savepoint_u_1, (void*)u_1_output.originPtr(),
                            u_1_output.strides().data(), u_1_output.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    ASSERT_TRUE(Storage::verify(u_1_output, u_1_input));

    // v_1 at savepoint_v_1
    serialboxSerializerRead(ser_read, "v", savepoint_v_1, (void*)v_1_output.originPtr(),
                            v_1_output.strides().data(), v_1_output.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    ASSERT_TRUE(Storage::verify(v_1_output, v_1_input));

    // field_6d at savepoint_6d
    serialboxSerializerRead(ser_read, "field_6d", savepoint_6d, (void*)field_6d_output.originPtr(),
                            field_6d_output.strides().data(), field_6d_output.strides().size());
    ASSERT_FALSE(this->hasErrorAndReset()) << this->getLastErrorMsg();
    ASSERT_TRUE(Storage::verify(field_6d_output, field_6d_input));

    serialboxSerializerDestroy(ser_read);
  }

  serialboxSavepointDestroy(savepoint1_t_1);
  serialboxSavepointDestroy(savepoint1_t_2);
  serialboxSavepointDestroy(savepoint_u_1);
  serialboxSavepointDestroy(savepoint_v_1);
  serialboxSavepointDestroy(savepoint_6d);
}
