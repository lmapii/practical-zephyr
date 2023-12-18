/** \file main.c */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

// Note: The lower_camel_case conversion for devicetree is not performed via the macros
// but must be done "manually" when providing the token.

/**
 * \name Node identifiers from `/aliases`.
 *
 * The `DT_ALIAS` macro is used to retrieve a node identifier (token) using a
 * property from the `/aliases` node:
 *
 * - `DT_ALIAS(alias_by_label)`  expands to `DT_N_ALIAS_alias_by_label`
 * - `DT_ALIAS(alias_by_path)`   expands to `DT_N_ALIAS_alias_by_path`
 * - `DT_ALIAS(alias_as_string)` expands to `DT_N_ALIAS_alias_as_string`
 *
 * In `devicetree_generated.h` you'll find that all of the above macros
 * provide the token `DT_N_S_node_with_props` and thus the node `/node_with_props`.
 *
 * \{ */
#define NODE_PROPS_ALIAS_BY_LABEL  DT_ALIAS(alias_by_label)
#define NODE_PROPS_ALIAS_BY_PATH   DT_ALIAS(alias_by_path)
#define NODE_PROPS_ALIAS_BY_STRING DT_ALIAS(alias_as_string)
/** \} */

/**
 * \name Node identifiers from `/chosen`.
 *
 * Similar to `DT_ALIASES`, `DT_CHOSEN` is used to retrieve a node identifier
 * (token) using a property from the `/chosen` node:
 *
 * - `DT_ALIAS(chosen_by_label)`  expands to `DT_N_ALIAS_chosen_by_label`
 * - `DT_ALIAS(chosen_by_path)`   expands to `DT_N_ALIAS_chosen_by_path`
 * - `DT_ALIAS(chosen_as_string)` expands to `DT_N_ALIAS_chosen_as_string`
 *
 * Again, all of the above macros provide the token `DT_N_S_node_with_props`
 * and thus the node `/node_with_props`.
 *
 * \{ */
#define NODE_PROPS_CHOSEN_BY_LABEL  DT_CHOSEN(chosen_by_label)
#define NODE_PROPS_CHOSEN_BY_PATH   DT_CHOSEN(chosen_by_path)
#define NODE_PROPS_CHOSEN_AS_STRING DT_CHOSEN(chosen_as_string)

/**
 * \brief Node identifier by label.
 *
 * `DT_NODELABEL` pastes `DT_N_NODELABEL` with the given label name,
 * resulting in `DT_N_NODELABEL_label_with_props`. This macro in turn provides the
 * token `DT_N_S_node_with_props` and thus the node `/node_with_props`.
 */
#define NODE_PROPS_BY_LABEL DT_NODELABEL(label_with_props)

/**
 * \brief Node identifier by path.
 *
 * `DT_PATH` is a recursive macro that takes a sequence of nodes, resulting in the
 * full path to the leaf node. The devicetree API assembles the corresponding token
 * starting with the root node `/` and thus `_S_`. For each node, a forward
 * slash `/` is inserted.
 *
 * The given `node_with_props` results in `DT_N_S_node_with_props`
 * and thus the node `/node_with_props`.
 */
#define NODE_PROPS_BY_PATH DT_PATH(node_with_props)

// We'll use the following structure to read /node_with_props's primitive values.
typedef struct
{
    bool boolean_exists;
    int32_t int_value;
    char *string_value;
    int32_t enum_int_value;
    char *enum_string_value;
} values_t;

/**
 * \brief String value as token.
 *
 * For properties of type `string` we can use `DT_STRING_TOKEN` to get the the
 * string's value as token and, e.g., use it as variable names or field names.
 * For our `string = string_value: "foo bar baz"` property, this resolves to
 * `foo_bar_baz`.
 *
 * `DT_STRING_UPPER_TOKEN` would resolve to the string's token in UPPER_CASE.
 */
#define STRING_TOKEN DT_STRING_TOKEN(NODE_PROPS_BY_PATH, string) // foo_bar_baz

static void print_basic_values(void)
{
    const values_t values = {
        .boolean_exists    = DT_PROP(NODE_PROPS_ALIAS_BY_LABEL, existent_boolean),
        .int_value         = DT_PROP(NODE_PROPS_ALIAS_BY_PATH, int),
        .string_value      = DT_PROP(NODE_PROPS_ALIAS_BY_STRING, string),
        .enum_int_value    = DT_PROP(NODE_PROPS_CHOSEN_BY_PATH, enum_int),
        .enum_string_value = DT_PROP(NODE_PROPS_CHOSEN_BY_LABEL, enum_string),
    };

    printk("values = {\n");
    printk("  .boolean_exists     = %d\n", values.boolean_exists);    // = 1
    printk("  .int_value          = %d\n", values.int_value);         // = 1
    printk("  .string_value       = %s\n", values.string_value);      // = "foo bar baz"
    printk("  .enum_int_value     = %d\n", values.enum_int_value);    // = 200
    printk("  .enum_string_value  = %s\n", values.enum_string_value); // = "whatever"
    printk("}\n");

    // DT_ENUM_IDX resolves to DT_N_S_node_with_props_P_enum_int_ENUM_IDX
    // "The index within 'enum_int' of the selected value '200' is 1."
    printk(
        "The index within 'enum_int' of the selected value '%d' is %d.\n",
        values.enum_int_value,
        DT_ENUM_IDX(NODE_PROPS_CHOSEN_AS_STRING, enum_int));

    // DT_ENUM_IDX resolves to DT_N_S_node_with_props_P_enum_string_ENUM_IDX
    // "The index within 'enum_string' of the selected value 'whatever' is 0."
    printk(
        "The index within 'enum_string' of the selected value '%s' is %d.\n",
        values.enum_string_value,
        DT_ENUM_IDX(NODE_PROPS_CHOSEN_AS_STRING, enum_string));

    // STRING_TOKEN resolves to DT_N_S_node_with_props_P_string_STRING_TOKEN
    // and thus the variable name foo_bar_baz.
    uint8_t STRING_TOKEN = 0U;
    STRING_TOKEN += 1U;
    printk("STRING_TOKEN = %d\n", STRING_TOKEN); // = 1
}

/**
 * \brief Macro used as function parameter for `FOREACH` macros.
 *
 * To use one of the `_FOREACH` macros provided for arrays, another macro
 * with the format `fn(node_id, prop, idx)` must be provided for the expansion.
 * This macro simply prints the index and string value of the given node's property.
 */
#define PRINTK_STRING(node_id, prop, idx)                                \
    do                                                                   \
    {                                                                    \
        printk("[%d] -- %s\n", idx, DT_PROP_BY_IDX(node_id, prop, idx)); \
    } while (0);

static void print_array_values(void)
{
    // cell_array = {10 /* 0xa */, 11 /* 0xb */, 12 /* 0xc */};
    const uint32_t cell_array[] = DT_PROP(NODE_PROPS_BY_LABEL, array);
    // bytestring = {18 /* 0x12 */, 52 /* 0x34 */};
    const uint8_t bytestring[] = DT_PROP(NODE_PROPS_BY_PATH, uint8_array);

    const size_t cell_array_exp_length = DT_PROP_LEN(NODE_PROPS_BY_LABEL, array);
    const size_t bytestring_exp_length = DT_PROP_LEN(NODE_PROPS_BY_LABEL, uint8_array);

    if ((cell_array_exp_length != (sizeof(cell_array) / sizeof(uint32_t))) ||
        (bytestring_exp_length != (sizeof(bytestring) / sizeof(uint8_t))))
    {
        // This is unreachable code, since the `_LEN` macro matches the
        // number of elements in the generated initializer list.
        printk("Something's wrong!\n");
    }
    else
    {
        printk("Values in `array`:\n");
        for (size_t i = 0U; i < cell_array_exp_length; i++)
        {
            printk("  [%d] = %u\n", i, cell_array[i]);
        }
        // [0] = 10
        // [1] = 11
        // [2] = 12

        printk("Values in `uint8_array`:\n");
        for (size_t i = 0U; i < bytestring_exp_length; i++)
        {
            printk("  [%d] = 0x%x\n", i, bytestring[i]);
        }
        // [0] = 0x12
        // [1] = 0x34
    }

    // This expands to one printk statement for each element in /node_with_props's
    // property string-array _at compile-time_.
    DT_FOREACH_PROP_ELEM(NODE_PROPS_BY_LABEL, string_array, PRINTK_STRING);
    // [0] -- foo
    // [1] -- bar
    // [2] -- baz
}

// Identifier of /node_refs.
#define NODE_REFS DT_PATH(node_refs)

/**
 * \name Node identifiers `phandle` and `phandles` types.
 *
 * Since both, `phandle` and `phandles` generate the same devicetree macros,
 * the same API macro `DT_PHANDLE_BY_IDX` can be used to get the node identifier
 * for the phandles. The macro `DT_PHANDLE` is available for the singular
 * `phandle` type, but simply redefines `DT_PHANDLE_BY_IDX` for the index 0.
 *
 * The retrieved phandles expand to `DT_N_S_node_a` and `DT_N_S_node_b` accordingly.
 *
 * Instead of retrieving the _phandle_, other API macros such as `DT_PROP_BY_PHANDLE`
 * are available that allow accessing the corresponding values directly.
 *
 * \{ */
#define NODE_A_PHANDLES         DT_PHANDLE_BY_IDX(NODE_REFS, phandle_array, 0)
#define NODE_B_PHANDLES         DT_PHANDLE_BY_IDX(NODE_REFS, phandle_array, 1)
#define NODE_A_PHANDLE_BY_LABEL DT_PHANDLE(NODE_REFS, phandle_by_label)
#define NODE_A_PHANDLE_BY_PATH  DT_PHANDLE(NODE_REFS, phandle_by_path)
/** \} */

/**
 * \deprecated
 * \brief Node identifier from `phandle` using `DT_PROP`
 *
 * For the singular `phandle` type, it is also possible to read the phandle using
 * the `DT_PROP` macro. It is, however, recommended to use `DT_PHANDLE` instead.
 *
 * \{ */
#define NODE_A_BY_PROP_AND_LABEL DT_PROP(NODE_REFS, phandle_by_label)
#define NODE_A_BY_PROP_AND_PATH  DT_PROP(NODE_REFS, phandle_by_path)
/** \} */

void print_phandle_values(void)
{
    // Properties can be accessed via the node identifier retrieved from the phandle and DT_PROP.
    uint32_t val_from_prop = DT_PROP(NODE_A_PHANDLE_BY_LABEL, dummy_value);
    // Typically, DT_PROP_BY_PHANDLE and DT_PROP_BY_PHANDLE_IDX are used instead.
    uint32_t val_from_phandle_by_label = DT_PROP_BY_PHANDLE(NODE_REFS, phandle_by_label, dummy_value);
    uint32_t val_from_phandle_by_path  = DT_PROP_BY_PHANDLE(NODE_REFS, phandle_by_path, dummy_value);
    uint32_t val_from_phandles         = DT_PROP_BY_PHANDLE_IDX(NODE_REFS, phandles, 0, dummy_value);

    // The following doesn't compile since node_b, which is at index 1,
    // does not have a property called `dummy-value`.
    // DT_PROP_BY_PHANDLE_IDX(NODE_REFS, phandles, 1, dummy_value);

    // clang-format off
    if ((val_from_prop == val_from_phandle_by_label) &&
        (val_from_phandle_by_label == val_from_phandle_by_path) &&
        (val_from_phandle_by_path == val_from_phandles) &&
        (val_from_phandles == 0xC0FFEE))
    // clang-format on
    {
        // The condition is always true.
        printk("Time for 0xc0ffee!\n");
    }
    else
    {
        // This is never executed.
        printk("Something went wrong ...\n");
    }
}

// Generic structure mapping to the bindings for label_a and label_b.
typedef struct
{
    uint32_t cell_one;
    uint32_t cell_two;
} node_spec_t;

/**
 * \brief Mapping of a `phandle-array` entry to node_spec_t.
 *
 * `phandle-array`s contain _phandles_ and therefore obviously don't provide an
 * initializer list since it is not possible to assign tokens. Instead,
 * `phandle-array`s are always accessed by
 * index.
 *
 * Each phandle in a `phandle-array` is followed by a predefined number of
 * specifier cells. Each specifier cell is in turn assigned a name in the node's
 * binding, and is thus also accessed via its name.
 *
 * E.g., for the following:
 * `phandle-array-of-refs = <&{/node_a} 1 2>, <&label_b 3>;`
 *
 * - `idx` 0 refers to the specifier `<&{/node_a} 1 2>`, and
 * - `idx` 1 refers to the specifier `<&label_b 3>`.
 *
 * For both specifiers, the _names_ of the cells must be known by the application.
 * Typically, all specifiers within a `phandle-array` have the same type. The node
 * types for `/node_a` and `/node_b` in this example are also compatible, with the
 * only difference that `node_b` does not have a specifier cell named
 * `name_of_cell_two`, which is why the `_OR` macro is used.
 */
#define NODE_DT_SPEC_GET_BY_IDX(node_id, prop, idx)                            \
    {                                                                          \
        .cell_one = DT_PHA_BY_IDX(node_id, prop, idx, name_of_cell_one),       \
        .cell_two = DT_PHA_BY_IDX_OR(node_id, prop, idx, name_of_cell_two, 0), \
    }

void print_phandle_array_values(void)
{
    // Metadata passed via `phandle-array` types is retrieved using the specifier cell's name.
    (void) DT_PHA_BY_IDX(NODE_REFS, phandle_array_of_refs, 0, name_of_cell_one); // 1
    (void) DT_PHA_BY_IDX(NODE_REFS, phandle_array_of_refs, 0, name_of_cell_two); // 2
    (void) DT_PHA_BY_IDX(NODE_REFS, phandle_array_of_refs, 1, name_of_cell_one); // 1

    // The following does not compile since index 1 refers to node_b.
    // The binding for node_b does not have a specifier cell named "name-of-cell-two".
    // DT_PHA_BY_IDX(NODE_REFS, phandle_array_of_refs, 1, name_of_cell_two);

    // node_a = {.cell_one = 1, .cell_two = 2};
    node_spec_t node_a = NODE_DT_SPEC_GET_BY_IDX(NODE_REFS, phandle_array_of_refs, 0);
    // node_b = {.cell_one = 1, .cell_two = 0};
    node_spec_t node_b = NODE_DT_SPEC_GET_BY_IDX(NODE_REFS, phandle_array_of_refs, 1);

    // Notice that due to the usage of `DT_PHA_BY_IDX_OR` the value for cell_two can be set
    // even though no macro exists! The `IS_ENABLED` macro in Zephyr performs this "magic".

    (void) node_a;
    (void) node_b;
}

#define SLEEP_TIME_MS 100U

void main(void)
{
    printk("Message in a bottle.\n");

    print_basic_values();
    print_array_values();
    print_phandle_values();
    print_phandle_array_values();

    // Real-world example:
    printk("The configured UART baud rate is %d bits/s.\n", DT_PROP(DT_PATH(soc, uart_40002000), current_speed));

    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
