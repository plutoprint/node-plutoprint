const fs = require("fs");
const path = require("path");

if(process.argv.includes("--inc")) {
  if(process.env.PLUTOBOOK_INC) {
    process.stdout.write(process.env.PLUTOBOOK_INC);
    process.exit(0);
  }

  const INC_SEARCH_DIRS = [
    '/opt/homebrew/include/plutobook',
    '/usr/local/include/plutobook',
    '/usr/include/plutobook', 
  ]

  for(const INC_DIR of INC_SEARCH_DIRS) {
    if(fs.existsSync(INC_DIR)) {
      process.stdout.write(INC_DIR);
      process.exit(0);
    }
  }
}

if(process.argv.includes("--lib")) {
  if(process.env.PLUTOBOOK_LIB) {
    process.stdout.write(process.env.PLUTOBOOK_LIB);
    process.exit(0);
  }

  const LIB_SEARCH_DIRS = [
    '/opt/homebrew/lib',
    '/usr/local/lib/x86_64-linux-gnu',
    '/usr/local/lib',
    '/usr/lib/x86_64-linux-gnu',
    '/usr/lib',
  ];

  const LIB_NAMES = [
    'libplutobook.dylib',
    'libplutobook.so',
    'libplutobook.a'
  ];

  for(const LIB_DIR of LIB_SEARCH_DIRS) {
    for(const LIB_NAME of LIB_NAMES) {
      if(fs.existsSync(path.join(LIB_DIR, LIB_NAME))) {
        process.stdout.write(`-L${LIB_DIR} -lplutobook`);
        process.exit(0);
      }
    }
  }

  process.stdout.write('-lplutobook');
  process.exit(0);
}
