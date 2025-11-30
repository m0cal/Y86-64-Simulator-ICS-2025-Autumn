project('Y86-64—Simulator', 'cpp',
  version : '0.1.0',
  default_options : ['cpp_std=c++23']
)

fs = import('fs')

catch2_dep = dependency('catch2-with-main', required : true)

core_sources = files(
  # 'src/cpu.cc',
  # 'src/memory.cc', # add your file here
)

main_source = files('src/main.cc') 

inc_dir = include_directories('src')

core_lib = static_library('core',
  core_sources,
  include_directories : inc_dir
)

executable('y86-64-sim',
  main_source,
  link_with : core_lib,
  include_directories : inc_dir,
  install : true
)

test_files = files(
  'dev_tests/main_test.cc'
  # 'dev_tests/cpu_test.cc',
)

foreach t : test_files
  test_name = fs.stem(t)
  test_exe = executable(
    test_name,
    t,
    link_with : core_lib,
    include_directories : inc_dir,
    dependencies : catch2_dep
  )
  test(test_name, test_exe)
endforeach
