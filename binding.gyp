{
  "targets": [
    {
      "target_name": "plutoprint",
      "sources": ["plutoprint.c"],
      "include_dirs": [
        "<!(node find-plutobook.js --inc)"
      ],
      "libraries": [
        "<!(node find-plutobook.js --lib)"
      ],
      "xcode_settings": {
        "OTHER_CFLAGS": ["-Wno-missing-field-initializers"]
      }
    }
  ]
}
