#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "leveldb::leveldb" for configuration ""
set_property(TARGET leveldb::leveldb APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(leveldb::leveldb PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libleveldb.so.1.23.0"
  IMPORTED_SONAME_NOCONFIG "libleveldb.so.1"
  )

list(APPEND _cmake_import_check_targets leveldb::leveldb )
list(APPEND _cmake_import_check_files_for_leveldb::leveldb "${_IMPORT_PREFIX}/lib/libleveldb.so.1.23.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
