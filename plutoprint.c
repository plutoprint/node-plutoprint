#include <node_api.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <plutobook.h>

static bool get_callback_info(napi_env env, napi_callback_info info, size_t* argc, napi_value* argv, napi_value* thisArg, size_t required, size_t optional)
{
    size_t provided = required + optional;
    napi_get_cb_info(env, info, &provided, argv, thisArg, NULL);

    size_t expected = required + optional;
    if(expected == 0 && provided > 0) {
        napi_throw_type_error(env, NULL, "No arguments expected");
        return false;
    }

    if(provided < required) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected at least %zu argument%s, got %zu", required, required == 1 ? "" : "s", provided);
        napi_throw_type_error(env, NULL, msg);
        return false;
    }

    if(provided > expected) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected at most %zu argument%s, got %zu", expected, expected == 1 ? "" : "s", provided);
        napi_throw_type_error(env, NULL, msg);
        return false;
    }

    if(argc)
        *argc = provided;
    return true;
}

static const char* type_name(napi_valuetype type)
{
    switch(type) {
    case napi_undefined:
        return "undefined";
    case napi_null:
        return "null";
    case napi_boolean:
        return "boolean";
    case napi_number:
        return "number";
    case napi_string:
        return "string";
    case napi_symbol:
        return "symbol";
    case napi_object:
        return "object";
    case napi_function:
        return "function";
    case napi_external:
        return "external";
    case napi_bigint:
        return "bigint";
    default:
        return "unknown";
    }
}

static void throw_argument_type_error(napi_env env, napi_value* argv, size_t argi, napi_valuetype expected)
{
    napi_valuetype provided;
    napi_typeof(env, argv[argi], &provided);

    char msg[128];
    snprintf(msg, sizeof(msg), "Argument %zu must be %s, not %s", argi + 1, type_name(expected), type_name(provided));
    napi_throw_type_error(env, NULL, msg);
}

static bool get_string_value(napi_env env, napi_value value, char** result)
{
    size_t size;
    napi_status status = napi_get_value_string_utf8(env, value, NULL, 0, &size);
    if(status != napi_ok) {
        return false;
    }

    char* buffer = malloc(size + 1);
    napi_get_value_string_utf8(env, value, buffer, size + 1, &size);
    *result = buffer;
    return true;
}

static char* get_string_argument(napi_env env, napi_value* argv, size_t argi)
{
    char* result;
    if(!get_string_value(env, argv[argi], &result)) {
        throw_argument_type_error(env, argv, argi, napi_string);
        return NULL;
    }

    return result;
}

static bool striequals(const char* a, const char* b)
{
    while(*a && *b) {
        if(tolower(*a) != tolower(*b))
            return false;
        ++a;
        ++b;
    }

    return (*a == '\0' && *b == '\0');
}

static bool string_option_func(napi_env env, napi_value property, const char* name, void* result)
{
    if(get_string_value(env, property, result)) {
        return true;
    }

    napi_valuetype type;
    napi_typeof(env, property, &type);

    char msg[128];
    snprintf(msg, sizeof(msg), "Property `%s` must be string, not %s", name, type_name(type));
    napi_throw_type_error(env, NULL, msg);
    return false;
}

static bool integer_option_func(napi_env env, napi_value property, const char* name, void* result)
{
    if(napi_get_value_int64(env, property, result) == napi_ok) {
        return true;
    }

    napi_valuetype type;
    napi_typeof(env, property, &type);

    char msg[128];
    snprintf(msg, sizeof(msg), "Property `%s` must be number, not %s", name, type_name(type));
    napi_throw_type_error(env, NULL, msg);
    return false;
}

static bool media_option_func(napi_env env, napi_value property, const char* name, void* result)
{
    char* value;
    if(!string_option_func(env, property, name, &value))
        return false;
    struct {
        const char* name;
        plutobook_media_type_t value;
    } table[] = {
        {"print", PLUTOBOOK_MEDIA_TYPE_PRINT},
        {"screen", PLUTOBOOK_MEDIA_TYPE_SCREEN},
        {NULL}
    };

    for(int i = 0; table[i].name; ++i) {
        if(striequals(table[i].name, value)) {
            *(plutobook_media_type_t*)(result) = table[i].value;
            free(value);
            return true;
        }
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "Property `%s` has invalid value \"%s\"", name, value);
    napi_throw_type_error(env, NULL, msg);
    free(value);
    return false;
}

static bool size_option_func(napi_env env, napi_value property, const char* name, void* result)
{
    char* value;
    if(!string_option_func(env, property, name, &value))
        return false;
    struct {
        const char* name;
        plutobook_page_size_t value;
    } table[] = {
        {"a3", PLUTOBOOK_PAGE_SIZE_A3},
        {"a4", PLUTOBOOK_PAGE_SIZE_A4},
        {"a5", PLUTOBOOK_PAGE_SIZE_A5},
        {"b4", PLUTOBOOK_PAGE_SIZE_B4},
        {"b5", PLUTOBOOK_PAGE_SIZE_B5},
        {"letter", PLUTOBOOK_PAGE_SIZE_LETTER},
        {"legal", PLUTOBOOK_PAGE_SIZE_LEGAL},
        {"ledger", PLUTOBOOK_PAGE_SIZE_LEDGER},
        {NULL}
    };

    for(int i = 0; table[i].name; ++i) {
        if(striequals(table[i].name, value)) {
            *(plutobook_page_size_t*)(result) = table[i].value;
            free(value);
            return true;
        }
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "Property `%s` has invalid value \"%s\"", name, value);
    napi_throw_type_error(env, NULL, msg);
    free(value);
    return false;
}

static bool length_option_func(napi_env env, napi_value property, const char* name, void* result)
{
    if(napi_get_value_double(env, property, result) == napi_ok) {
        return true;
    }

    char* value;
    if(!get_string_value(env, property, &value)) {
        napi_valuetype type;
        napi_typeof(env, property, &type);

        char msg[128];
        snprintf(msg, sizeof(msg), "Property `%s` must be string or number, not %s", name, type_name(type));
        napi_throw_type_error(env, NULL, msg);
        return false;
    }

    char* end;
    double length = strtod(value, &end);
    static const struct {
        const char* name;
        const double factor;
    } table[] = {
        {"pt", PLUTOBOOK_UNITS_PT},
        {"pc", PLUTOBOOK_UNITS_PC},
        {"in", PLUTOBOOK_UNITS_IN},
        {"cm", PLUTOBOOK_UNITS_CM},
        {"mm", PLUTOBOOK_UNITS_MM},
        {"px", PLUTOBOOK_UNITS_PX},
        {NULL}
    };

    for(int i = 0; table[i].name; ++i) {
        if(striequals(table[i].name, end)) {
            *(double*)(result) = length * table[i].factor;
            free(value);
            return true;
        }
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "Property `%s` must be a valid length, not \"%s\"", name, value);
    napi_throw_type_error(env, NULL, msg);
    free(value);
    return false;
}

static bool date_option_func(napi_env env, napi_value property, const char* name, void* result)
{
    if(napi_get_date_value(env, property, result) == napi_ok) {
        return true;
    }

    napi_valuetype type;
    napi_typeof(env, property, &type);

    char msg[128];
    snprintf(msg, sizeof(msg), "Property `%s` must be date, not %s", name, type_name(type));
    napi_throw_type_error(env, NULL, msg);
    return false;
}

typedef bool (*option_func_t)(napi_env env, napi_value property, const char* name, void* result);

typedef struct {
    const char* name;
    option_func_t func;
    void* result;
} option_t;

static bool parse_options(napi_env env, napi_value* argv, size_t argc, size_t argi, option_t* options)
{
    napi_valuetype type;
    napi_typeof(env, argv[argi], &type);
    if(type != napi_object) {
        throw_argument_type_error(env, argv, argi, napi_object);
        return false;
    }

    for(int i = 0; options[i].name; ++i) {
        bool has_property;
        napi_has_named_property(env, argv[argi], options[i].name, &has_property);
        if(has_property) {
            napi_value property;
            napi_get_named_property(env, argv[argi], options[i].name, &property);
            if(!options[i].func(env, property, options[i].name, options[i].result)) {
                return false;
            }
        }
    }

    return true;
}

static napi_ref BookClass_Ref;

static napi_value CreateBook(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1];
    if(!get_callback_info(env, info, &argc, argv, NULL, 0, 1)) {
        return NULL;
    }

    napi_value BookClass;
    napi_get_reference_value(env, BookClass_Ref, &BookClass);

    napi_value instance;
    napi_new_instance(env, BookClass, argc, argv, &instance);
    return instance;
}

static void BookClass_Finalize(napi_env env, void* data, void* hint)
{
    plutobook_destroy(data);
}

static void set_date_metadata(plutobook_t* book, plutobook_pdf_metadata_t metadata, double date)
{
    char value[sizeof("2025-12-11T05:36:01Z")];
    time_t time = (time_t)(date / 1000);
    strftime(value, sizeof(value), "%FT%TZ", gmtime(&time));
    plutobook_set_metadata(book, metadata, value);
}

static napi_value BookClass_Constructor(napi_env env, napi_callback_info info)
{
    napi_value new_target;
    napi_get_new_target(env, info, &new_target);
    if(new_target == NULL) {
        return CreateBook(env, info);
    }

    size_t argc = 1;
    napi_value argv[1];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 0, 1)) {
        return NULL;
    }

    plutobook_page_size_t size = PLUTOBOOK_PAGE_SIZE_A4;
    plutobook_media_type_t media = PLUTOBOOK_MEDIA_TYPE_PRINT;

    double width = -1;
    double height = -1;

    double margin = 72;
    double marginTop = -1;
    double marginRight = -1;
    double marginBottom = -1;
    double marginLeft = -1;

    char* title = NULL;
    char* subject = NULL;
    char* author = NULL;
    char* keywords = NULL;
    char* creator = NULL;

    double creationDate = -1;
    double modificationDate = -1;

    if(argc == 1) {
        option_t options[] = {
            {"size", size_option_func, &size},
            {"media", media_option_func, &media},
            {"width", length_option_func, &width},
            {"height", length_option_func, &height},
            {"margin", length_option_func, &margin},
            {"marginTop", length_option_func, &marginTop},
            {"marginRight", length_option_func, &marginRight},
            {"marginBottom", length_option_func, &marginBottom},
            {"marginLeft", length_option_func, &marginLeft},
            {"title", string_option_func, &title},
            {"subject", string_option_func, &subject},
            {"author", string_option_func, &author},
            {"keywords", string_option_func, &keywords},
            {"creator", string_option_func, &creator},
            {"creationDate", date_option_func, &creationDate},
            {"modificationDate", date_option_func, &modificationDate},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 0, options)) {
            thisArg = NULL;
            goto cleanup;
        }
    }

    if(width != -1)
        size.width = width;
    if(height != -1) {
        size.height = height;
    }

    plutobook_page_margins_t margins = PLUTOBOOK_MAKE_PAGE_MARGINS(margin, margin, margin, margin);
    if(marginTop != -1)
        margins.top = marginTop;
    if(marginRight != -1)
        margins.right = marginRight;
    if(marginBottom != -1)
        margins.bottom = marginBottom;
    if(marginLeft != -1) {
        margins.left = marginLeft;
    }

    plutobook_t* book = plutobook_create(size, margins, media);
    if(title)
        plutobook_set_metadata(book, PLUTOBOOK_PDF_METADATA_TITLE, title);
    if(subject)
        plutobook_set_metadata(book, PLUTOBOOK_PDF_METADATA_SUBJECT, subject);
    if(author)
        plutobook_set_metadata(book, PLUTOBOOK_PDF_METADATA_AUTHOR, author);
    if(keywords)
        plutobook_set_metadata(book, PLUTOBOOK_PDF_METADATA_KEYWORDS, keywords);
    if(creator) {
        plutobook_set_metadata(book, PLUTOBOOK_PDF_METADATA_CREATOR, creator);
    }

    if(creationDate != -1)
        set_date_metadata(book, PLUTOBOOK_PDF_METADATA_CREATION_DATE, creationDate);
    if(modificationDate != -1) {
        set_date_metadata(book, PLUTOBOOK_PDF_METADATA_MODIFICATION_DATE, modificationDate);
    }

    napi_wrap(env, thisArg, book, BookClass_Finalize, NULL, NULL);
cleanup:
    free(title);
    free(subject);
    free(author);
    free(keywords);
    free(creator);
    return thisArg;
}

static napi_value Book_PageCount(napi_env env, napi_callback_info info)
{
    napi_value thisArg;
    if(!get_callback_info(env, info, NULL, NULL, &thisArg, 0, 0)) {
        return NULL;
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    napi_value result;
    napi_create_uint32(env, plutobook_get_page_count(book), &result);
    return result;
}

static napi_value Book_DocumentWidth(napi_env env, napi_callback_info info)
{
    napi_value thisArg;
    if(!get_callback_info(env, info, NULL, NULL, &thisArg, 0, 0)) {
        return NULL;
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    napi_value result;
    napi_create_double(env, plutobook_get_document_width(book), &result);
    return result;
}

static napi_value Book_DocumentHeight(napi_env env, napi_callback_info info)
{
    napi_value thisArg;
    if(!get_callback_info(env, info, NULL, NULL, &thisArg, 0, 0)) {
        return NULL;
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    napi_value result;
    napi_create_double(env, plutobook_get_document_height(book), &result);
    return result;
}

static napi_value Book_ViewportWidth(napi_env env, napi_callback_info info)
{
    napi_value thisArg;
    if(!get_callback_info(env, info, NULL, NULL, &thisArg, 0, 0)) {
        return NULL;
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    napi_value result;
    napi_create_double(env, plutobook_get_viewport_width(book), &result);
    return result;
}

static napi_value Book_ViewportHeight(napi_env env, napi_callback_info info)
{
    napi_value thisArg;
    if(!get_callback_info(env, info, NULL, NULL, &thisArg, 0, 0)) {
        return NULL;
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    napi_value result;
    napi_create_double(env, plutobook_get_viewport_height(book), &result);
    return result;
}

static napi_value Book_LoadUrl(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 1, 1)) {
        return NULL;
    }

    char* url = get_string_argument(env, argv, 0);
    if(url == NULL) {
        return NULL;
    }

    char* userStyle = NULL;
    char* userScript = NULL;

    if(argc == 2) {
        option_t options[] = {
            {"userStyle", string_option_func, &userStyle},
            {"userScript", string_option_func, &userScript},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 1, options)) {
            thisArg = NULL;
            goto cleanup;
        }
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    const char* user_style = userStyle ? userStyle : "";
    const char* user_script = userScript ? userScript : "";

    if(!plutobook_load_url(book, url, user_style, user_script)) {
        napi_throw_error(env, NULL, plutobook_get_error_message());
        thisArg = NULL;
    }

cleanup:
    free(url);
    free(userStyle);
    free(userScript);
    return thisArg;
}

static napi_value Book_LoadHtml(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 1, 1)) {
        return NULL;
    }

    char* content = get_string_argument(env, argv, 0);
    if(content == NULL) {
        return NULL;
    }

    char* userStyle = NULL;
    char* userScript = NULL;
    char* baseUrl = NULL;

    if(argc == 2) {
        option_t options[] = {
            {"userStyle", string_option_func, &userStyle},
            {"userScript", string_option_func, &userScript},
            {"baseUrl", string_option_func, &baseUrl},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 1, options)) {
            thisArg = NULL;
            goto cleanup;
        }
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    const char* user_style = userStyle ? userStyle : "";
    const char* user_script = userScript ? userScript : "";
    const char* base_url = baseUrl ? baseUrl : "";

    if(!plutobook_load_html(book, content, -1, user_style, user_script, base_url)) {
        napi_throw_error(env, NULL, plutobook_get_error_message());
        thisArg = NULL;
    }

cleanup:
    free(userStyle);
    free(userScript);
    free(baseUrl);
    return thisArg;
}

static napi_value Book_LoadXml(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 1, 1)) {
        return NULL;
    }

    char* content = get_string_argument(env, argv, 0);
    if(content == NULL) {
        return NULL;
    }

    char* userStyle = NULL;
    char* userScript = NULL;
    char* baseUrl = NULL;

    if(argc == 2) {
        option_t options[] = {
            {"userStyle", string_option_func, &userStyle},
            {"userScript", string_option_func, &userScript},
            {"baseUrl", string_option_func, &baseUrl},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 1, options)) {
            thisArg = NULL;
            goto cleanup;
        }
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    const char* user_style = userStyle ? userStyle : "";
    const char* user_script = userScript ? userScript : "";
    const char* base_url = baseUrl ? baseUrl : "";

    if(!plutobook_load_xml(book, content, -1, user_style, user_script, base_url)) {
        napi_throw_error(env, NULL, plutobook_get_error_message());
        thisArg = NULL;
    }

cleanup:
    free(userStyle);
    free(userScript);
    free(baseUrl);
    return thisArg;
}

static bool get_buffer_argument(napi_env env, napi_value* argv, size_t argi, void** buffer, size_t* length)
{
    if(napi_get_buffer_info(env, argv[argi], buffer, length) == napi_ok) {
        return true;
    }

    napi_valuetype type;
    napi_typeof(env, argv[argi], &type);

    char msg[128];
    snprintf(msg, sizeof(msg), "Argument %zu must be buffer, not %s", argi + 1, type_name(type));
    napi_throw_type_error(env, NULL, msg);
    return false;
}

static napi_value Book_LoadData(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 1, 1)) {
        return NULL;
    }

    void* buffer;
    size_t length;
    if(!get_buffer_argument(env, argv, 0, &buffer, &length)) {
        return NULL;
    }

    char* mimeType = NULL;
    char* textEncoding = NULL;
    char* userStyle = NULL;
    char* userScript = NULL;
    char* baseUrl = NULL;

    if(argc == 2) {
        option_t options[] = {
            {"mimeType", string_option_func, &mimeType},
            {"textEncoding", string_option_func, &textEncoding},
            {"userStyle", string_option_func, &userStyle},
            {"userScript", string_option_func, &userScript},
            {"baseUrl", string_option_func, &baseUrl},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 1, options)) {
            thisArg = NULL;
            goto cleanup;
        }
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    const char* mime_type = mimeType ? mimeType : "";
    const char* text_encoding = textEncoding ? textEncoding : "";
    const char* user_style = userStyle ? userStyle : "";
    const char* user_script = userScript ? userScript : "";
    const char* base_url = baseUrl ? baseUrl : "";

    if(!plutobook_load_data(book, buffer, length, mime_type, text_encoding, user_style, user_script, base_url)) {
        napi_throw_error(env, NULL, plutobook_get_error_message());
        thisArg = NULL;
    }

cleanup:
    free(mimeType);
    free(textEncoding);
    free(userStyle);
    free(userScript);
    free(baseUrl);
    return thisArg;
}

static napi_value Book_LoadImage(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 1, 1)) {
        return NULL;
    }

    void* buffer;
    size_t length;
    if(!get_buffer_argument(env, argv, 0, &buffer, &length)) {
        return NULL;
    }

    char* mimeType = NULL;
    char* textEncoding = NULL;
    char* userStyle = NULL;
    char* userScript = NULL;
    char* baseUrl = NULL;

    if(argc == 2) {
        option_t options[] = {
            {"mimeType", string_option_func, &mimeType},
            {"textEncoding", string_option_func, &textEncoding},
            {"userStyle", string_option_func, &userStyle},
            {"userScript", string_option_func, &userScript},
            {"baseUrl", string_option_func, &baseUrl},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 1, options)) {
            thisArg = NULL;
            goto cleanup;
        }
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    const char* mime_type = mimeType ? mimeType : "";
    const char* text_encoding = textEncoding ? textEncoding : "";
    const char* user_style = userStyle ? userStyle : "";
    const char* user_script = userScript ? userScript : "";
    const char* base_url = baseUrl ? baseUrl : "";

    if(!plutobook_load_image(book, buffer, length, mime_type, text_encoding, user_style, user_script, base_url)) {
        napi_throw_error(env, NULL, plutobook_get_error_message());
        thisArg = NULL;
    }

cleanup:
    free(mimeType);
    free(textEncoding);
    free(userStyle);
    free(userScript);
    free(baseUrl);
    return thisArg;
}

static napi_value Book_WriteToPdf(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 1, 1)) {
        return NULL;
    }

    char* path = get_string_argument(env, argv, 0);
    if(path == NULL) {
        return NULL;
    }

    napi_value result = NULL;

    int64_t pageStart = PLUTOBOOK_MIN_PAGE_COUNT;
    int64_t pageEnd = PLUTOBOOK_MAX_PAGE_COUNT;
    int64_t pageStep = 1;

    if(argc == 2) {
        option_t options[] = {
            {"pageStart", integer_option_func, &pageStart},
            {"pageEnd", integer_option_func, &pageEnd},
            {"pageStep", integer_option_func, &pageStep},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 1, options)) {
            goto cleanup;
        }
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    if(!plutobook_write_to_pdf_range(book, path, pageStart, pageEnd, pageStep)) {
        napi_throw_error(env, NULL, plutobook_get_error_message());
        goto cleanup;
    }

    napi_get_undefined(env, &result);
cleanup:
    free(path);
    return result;
}

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} memory_stream_t;

static void memory_stream_init(memory_stream_t* stream)
{
    stream->data = NULL;
    stream->size = 0;
    stream->capacity = 0;
}

static void memory_stream_destroy(memory_stream_t* stream)
{
    free(stream->data);
}

static plutobook_stream_status_t stream_write_func(void* closure, const char* data, unsigned int length)
{
    memory_stream_t* stream = closure;

    size_t required_capacity = stream->size + length;
    if(required_capacity > stream->capacity) {
        size_t new_capacity = stream->capacity == 0 ? 128 : stream->capacity;
        while(new_capacity < required_capacity) {
            new_capacity *= 2;
        }

        char* new_data = realloc(stream->data, new_capacity);
        if(new_data == NULL)
            return PLUTOBOOK_STREAM_STATUS_WRITE_ERROR;
        stream->data = new_data;
        stream->capacity = new_capacity;
    }

    memcpy(stream->data + stream->size, data, length);
    stream->size += length;
    return PLUTOBOOK_STREAM_STATUS_SUCCESS;
}

static napi_value Book_WriteToPdfBuffer(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 0, 1)) {
        return NULL;
    }

    napi_value result = NULL;

    memory_stream_t stream;
    memory_stream_init(&stream);

    int64_t pageStart = PLUTOBOOK_MIN_PAGE_COUNT;
    int64_t pageEnd = PLUTOBOOK_MAX_PAGE_COUNT;
    int64_t pageStep = 1;

    if(argc == 1) {
        option_t options[] = {
            {"pageStart", integer_option_func, &pageStart},
            {"pageEnd", integer_option_func, &pageEnd},
            {"pageStep", integer_option_func, &pageStep},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 0, options)) {
            goto cleanup;
        }
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    if(!plutobook_write_to_pdf_stream_range(book, stream_write_func, &stream, pageStart, pageEnd, pageStep)) {
        napi_throw_error(env, NULL, plutobook_get_error_message());
        goto cleanup;
    }

    napi_create_buffer_copy(env, stream.size, stream.data, NULL, &result);
cleanup:
    memory_stream_destroy(&stream);
    return result;
}

static napi_value Book_WriteToPng(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 1, 1)) {
        return NULL;
    }

    char* path = get_string_argument(env, argv, 0);
    if(path == NULL) {
        return NULL;
    }

    napi_value result = NULL;

    int64_t width = -1;
    int64_t height = -1;

    if(argc == 2) {
        option_t options[] = {
            {"width", integer_option_func, &width},
            {"height", integer_option_func, &height},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 1, options)) {
            goto cleanup;
        }
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    if(!plutobook_write_to_png(book, path, width, height)) {
        napi_throw_error(env, NULL, plutobook_get_error_message());
        goto cleanup;
    }

    napi_get_undefined(env, &result);
cleanup:
    free(path);
    return result;
}

static napi_value Book_WriteToPngBuffer(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_value thisArg;
    if(!get_callback_info(env, info, &argc, argv, &thisArg, 0, 1)) {
        return NULL;
    }

    napi_value result = NULL;

    memory_stream_t stream;
    memory_stream_init(&stream);

    int64_t width = -1;
    int64_t height = -1;

    if(argc == 1) {
        option_t options[] = {
            {"width", integer_option_func, &width},
            {"height", integer_option_func, &height},
            {NULL}
        };

        if(!parse_options(env, argv, argc, 0, options)) {
            goto cleanup;
        }
    }

    plutobook_t* book;
    napi_unwrap(env, thisArg, (void**)&book);

    if(!plutobook_write_to_png_stream(book, stream_write_func, &stream, width, height)) {
        napi_throw_error(env, NULL, plutobook_get_error_message());
        goto cleanup;
    }

    napi_create_buffer_copy(env, stream.size, stream.data, NULL, &result);
cleanup:
    memory_stream_destroy(&stream);
    return result;
}

static void BookClass_Init(napi_env env, napi_value exports)
{
    const napi_property_descriptor properties[] = {
        {"pageCount", NULL, NULL, Book_PageCount, NULL, NULL, napi_default, NULL },
        {"documentWidth", NULL, NULL, Book_DocumentWidth, NULL, NULL, napi_default, NULL },
        {"documentHeight", NULL, NULL, Book_DocumentHeight, NULL, NULL, napi_default, NULL },
        {"viewportWidth", NULL, NULL, Book_ViewportWidth, NULL, NULL, napi_default, NULL },
        {"viewportHeight", NULL, NULL, Book_ViewportHeight, NULL, NULL, napi_default, NULL },
        {"loadUrl", NULL, Book_LoadUrl, NULL, NULL, NULL, napi_default, NULL },
        {"loadHtml", NULL, Book_LoadHtml, NULL, NULL, NULL, napi_default, NULL },
        {"loadXml", NULL, Book_LoadXml, NULL, NULL, NULL, napi_default, NULL },
        {"loadData", NULL, Book_LoadData, NULL, NULL, NULL, napi_default, NULL },
        {"loadImage", NULL, Book_LoadImage, NULL, NULL, NULL, napi_default, NULL },
        {"writeToPdf", NULL, Book_WriteToPdf, NULL, NULL, NULL, napi_default, NULL },
        {"writeToPdfBuffer", NULL, Book_WriteToPdfBuffer, NULL, NULL, NULL, napi_default, NULL },
        {"writeToPng", NULL, Book_WriteToPng, NULL, NULL, NULL, napi_default, NULL },
        {"writeToPngBuffer", NULL, Book_WriteToPngBuffer, NULL, NULL, NULL, napi_default, NULL },
    };

    size_t property_count = sizeof(properties) / sizeof(napi_property_descriptor);

    napi_value BookClass;
    napi_define_class(env, "Book", NAPI_AUTO_LENGTH, BookClass_Constructor, NULL, property_count, properties, &BookClass);
    napi_create_reference(env, BookClass, 1, &BookClass_Ref);
    napi_set_named_property(env, exports, "Book", BookClass);
}

#define EXPORT_STRING(name, string) do { \
    napi_value result; \
    napi_create_string_utf8(env, string, NAPI_AUTO_LENGTH, &result); \
    napi_set_named_property(env, exports, name, result); \
} while(0)

#define EXPORT_INTEGER(name, integer) do { \
    napi_value result; \
    napi_create_int64(env, integer, &result); \
    napi_set_named_property(env, exports, name, result); \
} while(0)

#define EXPORT_NUMBER(name, number) do { \
    napi_value result; \
    napi_create_double(env, number, &result); \
    napi_set_named_property(env, exports, name, result); \
} while(0)

#define EXPORT_FUNCTION(name, function) do { \
    napi_value result; \
    napi_create_function(env, name, NAPI_AUTO_LENGTH, function, NULL, &result); \
    napi_set_named_property(env, exports, name, result); \
} while(0)

napi_value Init(napi_env env, napi_value exports)
{
    BookClass_Init(env, exports);

    EXPORT_FUNCTION("createBook", CreateBook);

    EXPORT_STRING("plutobookVersion", plutobook_version_string());
    EXPORT_STRING("plutobookBuildInfo", plutobook_build_info());

    EXPORT_INTEGER("MIN_PAGE_COUNT", PLUTOBOOK_MIN_PAGE_COUNT);
    EXPORT_INTEGER("MAX_PAGE_COUNT", PLUTOBOOK_MAX_PAGE_COUNT);

    EXPORT_NUMBER("UNITS_PT", PLUTOBOOK_UNITS_PT);
    EXPORT_NUMBER("UNITS_PC", PLUTOBOOK_UNITS_PC);
    EXPORT_NUMBER("UNITS_IN", PLUTOBOOK_UNITS_IN);
    EXPORT_NUMBER("UNITS_CM", PLUTOBOOK_UNITS_CM);
    EXPORT_NUMBER("UNITS_MM", PLUTOBOOK_UNITS_MM);
    EXPORT_NUMBER("UNITS_PX", PLUTOBOOK_UNITS_PX);
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
