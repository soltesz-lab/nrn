set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${PROJECT_SOURCE_DIR}/test ${PROJECT_SOURCE_DIR}/src/nrnoc
                    ${PROJECT_SOURCE_DIR}/src/nrniv ${PROJECT_SOURCE_DIR}/src/oc)

# =============================================================================
# Test executables
# =============================================================================
set(TEST_SOURCES unit_tests/oc/hoc_interpreter.cpp)
add_executable(testneuron unit_tests/unit_test.cpp ${TEST_SOURCES})
target_compile_features(testneuron PUBLIC cxx_std_11)
target_link_libraries(testneuron Catch2::Catch2 nrniv_lib ${CMAKE_THREAD_LIBS_INIT})
if(NOT MINGW)
  target_link_libraries(testneuron ${CMAKE_DL_LIBS})
endif()

# =============================================================================
# Copy necessary hoc files to build directory if they have not been copied yet
# =============================================================================
add_custom_command(
  TARGET testneuron POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/share/lib
          ${PROJECT_BINARY_DIR}/share/nrn/lib)

# Note that DYLD_LIBRARY_PATH is not required and interfere with dlopen
set(TEST_ENV
    NEURONHOME=${PROJECT_BINARY_DIR}/share/nrn NRNHOME=${PROJECT_BINARY_DIR}
    LD_LIBRARY_PATH=${PROJECT_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH})
if(NRN_ENABLE_PYTHON)
  list(
    APPEND
      TEST_ENV
      PYTHONPATH=${PROJECT_BINARY_DIR}/lib/python:${PROJECT_SOURCE_DIR}/test/rxd:$ENV{PYTHONPATH})
endif()

set(TESTS "")

# =============================================================================
# Use add_test method and set environment for testneuron (NEURONHOME env variable needs to be set to
# run testneuron and other tests based on nrniv)
# =============================================================================
foreach(test_name testneuron)
  add_test(NAME ${test_name} COMMAND ${test_name})
  list(APPEND TESTS ${test_name})
endforeach()

# =============================================================================
# Add ringtest
# =============================================================================
set(RINGTEST_DIR ${PROJECT_SOURCE_DIR}/test/ringtest)
add_test(
  ringtest
  ${CMAKE_COMMAND}
  -Dexecutable=${CMAKE_BINARY_DIR}/bin/nrniv
  -Dexec_arg=ring.hoc
  -Dout_file=out.dat
  -Dref_file=out.dat.ref
  -Dwork_dir=${RINGTEST_DIR}
  -P
  ${PROJECT_SOURCE_DIR}/cmake/RunHOCTest.cmake)
list(APPEND TESTS ringtest)

# =============================================================================
# Add small hoc test
# =============================================================================
set(HOCTEST_DIR ${PROJECT_SOURCE_DIR}/test/hoc_tests/connect_dend)
if(NRN_ENABLE_CORENEURON)
  set(REF_FILE cell3soma.core.dat.ref)
else()
  set(REF_FILE cell3soma.dat.ref)
endif()
add_test(
  connect_dend
  ${CMAKE_COMMAND}
  -Dexecutable=${CMAKE_BINARY_DIR}/bin/nrniv
  -Dexec_arg=connect_dend.hoc
  -Dout_file=cell3soma.dat
  -Dref_file=${REF_FILE}
  -Dwork_dir=${HOCTEST_DIR}
  -P
  ${PROJECT_SOURCE_DIR}/cmake/RunHOCTest.cmake)
list(APPEND TESTS connect_dend)

# =============================================================================
# Add pytest
# =============================================================================
if(NRN_ENABLE_PYTHON AND PYTEST_FOUND)
  add_test(NAME basic_tests COMMAND ${PYTHON_EXECUTABLE} -m pytest ${PROJECT_SOURCE_DIR}/test/pynrn)
  add_custom_target(
    rxdmod ALL
    COMMAND
      ${CMAKE_COMMAND} -E env ${TEST_ENV} $ENV{SHELL} ${CMAKE_BINARY_DIR}/bin/nrnivmodl ./ecs
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/rxd)
  add_dependencies(rxdmod nrniv)
  add_test(NAME rxd_tests COMMAND ${PYTHON_EXECUTABLE} -m pytest ${PROJECT_SOURCE_DIR}/test/rxd)
  list(APPEND TESTS basic_tests rxd_tests)
  if(NRN_ENABLE_MPI)
    find_python_module(mpi4py)
    if(mpi4py_FOUND)
      add_test(
        NAME rxd_mpi_tests
        COMMAND
          ${MPIEXEC} -np 1 ${PYTHON_EXECUTABLE} -m pytest ${PROJECT_SOURCE_DIR}/test/rxd --mpi)
      list(APPEND TESTS rxd_mpi_tests)
    endif()
  endif()

  if(NRN_ENABLE_CORENEURON)
    file(COPY ${PROJECT_SOURCE_DIR}/test/coreneuron/mod DESTINATION ${PROJECT_BINARY_DIR}/test/)
    add_custom_target(
      coreneuron_mod ALL
      COMMAND
        ${CMAKE_COMMAND} -E env ${TEST_ENV} $ENV{SHELL} ${CMAKE_BINARY_DIR}/bin/nrnivmodl -coreneuron .
       WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test/mod)
    add_dependencies(coreneuron_mod nrniv nrniv-core)

    add_test(NAME coreneuron_direct_py COMMAND ${PYTHON_EXECUTABLE}
                                            ${PROJECT_SOURCE_DIR}/test/coreneuron/test_direct.py
                                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test/mod)
    add_test(NAME coreneuron_direct_hoc COMMAND ${CMAKE_BINARY_DIR}/bin/nrniv
                                            ${PROJECT_SOURCE_DIR}/test/coreneuron/test_direct.hoc
                                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test/mod)
    add_test(NAME coreneuron_spikes_py COMMAND ${PYTHON_EXECUTABLE}
                                            ${PROJECT_SOURCE_DIR}/test/coreneuron/test_spikes.py
                                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test/mod)
    add_test(NAME coreneuron_fornetcon_py COMMAND ${PYTHON_EXECUTABLE}
                                            ${PROJECT_SOURCE_DIR}/test/coreneuron/test_fornetcon.py
                                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test/mod)
    add_test(NAME coreneuron_datareturn_py COMMAND ${PYTHON_EXECUTABLE}
                                            ${PROJECT_SOURCE_DIR}/test/coreneuron/test_datareturn.py
                                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test/mod)
    list(APPEND TESTS coreneuron_direct_py coreneuron_direct_hoc
                      coreneuron_spikes_py coreneuron_fornetcon_py
                      coreneuron_datareturn_py)
    if(NRN_ENABLE_MPI)
      find_python_module(mpi4py)
      if(mpi4py_FOUND)
        add_test(
                NAME coreneuron_mpi_spikes_py  COMMAND ${MPIEXEC} -np 2 ${PYTHON_EXECUTABLE}
                                              ${PROJECT_SOURCE_DIR}/test/coreneuron/test_spikes.py mpi4py
                                      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test/mod)
        list(APPEND TESTS coreneuron_mpi_spikes_py)
      endif()
    endif()
  endif()
endif()
set_tests_properties(${TESTS} PROPERTIES ENVIRONMENT "${TEST_ENV}")
