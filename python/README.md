# sslogread

Python reader module for the Speedy Structured C++17 logging library

See [github repository](https://github.com/dfeneyrou/sslog) for more details.

## Installation

### From pypi
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shell
python3 -m pip install sslogread
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### From sources

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shell
git clone https://github.com/dfeneyrou/sslog
cd sslog
mkdir build && cd build
cmake .. -DSSLOG_BUILD_PYTHON_READER=ON
make -j
python3 -m pip install python/dist/sslogread-*.whl
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## sslogread.load

This function (the unique one of the module) is the mandatory first step to work with logs. <br>
It reads the meta information of the logs and creates the `sslogread.LogSession` objects with
enables logs manipulation.

### Declaration

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
def load(path)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

| Parameter name | Type     | Description                             |
|----------------|----------|-----------------------------------------|
| `path`         | `string` | The path of the log folder to represent |

The returned value is a `LogSession` object if the loading succeeded. In case of failure, an exception with textual explanation is thrown.

The `LogSession` objects contains only two methods that are described in the following chapters:
| Method name   | Description                                                                                                                                |
|---------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| `query`       | Filters logs and return them in a structured form. <br> This is the main service of the module.                                            |
| `get_strings` | Returns filtered unique strings. <br> This service is typically used to check user-defined consistency of category, name and unit strings. |


### Example

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
import sslogread

try:
    session = sslogread.load("/path/to/my/log_folder")
except Exception as e:
    print("Loading failed:", e)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## sslogread.LogSession.query

This is the main service of the LogSession object. Logs are first filtered then returned in a structured form.

### Declaration

The parameters are a list of dictionaries, each of them representing a filtering rule. <br>
A logic OR is performed between them

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
def LogSession.query({ level_min:"trace", level_max:"off", category:"", thread:"", format:"", must_have_buffer:False, arguments:[],
                     no_category:"", no_thread:"", no_format:"", must_not_have_buffer:False }, { ... }, ... )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A filter rule is a dictionary containing one or many of the following key-value pairs (logic AND between them):
| Parameter name    | Type          | Description                                                            | Default (accept all) |
|-------------------|---------------|------------------------------------------------------------------------|----------------------|
| `'level_min'`     | `string`      | The minimum log level                                                  | 'trace'              |
| `'level_max'`     | `string`      | The maximum log level (`off` is above all)                             | 'off'                |
| `'category'`      | `string`      | Filtering-in pattern on category                                       | ' '                  |
| `'thread'`        | `string`      | Filtering-in pattern on thread name                                    | ' '                  |
| `'format'`        | `string`      | Filtering-in pattern on format string                                  | ' '                  |
| `buffer_size_min` | `integer`     | Filtering-in if buffer length is equal or above                        | 0                    |
| `buffer_size_max` | `integer`     | Filtering-in if buffer length is equal or below                        | `maximum size (4GB)` |
| `'arguments'`     | `[ strings ]` | Filtering-in on arguments names and value <br> (!) No pattern matching | ' '                  |
| `'no_category'`   | `string`      | Filtering-out pattern on category                                      | ' '                  |
| `'no_thread'`     | `string`      | Filtering-out pattern on thread name                                   | ' '                  |
| `'no_format'`     | `string`      | Filtering-out pattern on format string                                 | ' '                  |

> [!IMPORTANT]
> Filtering on arguments is not pattern based. Names and values must be exact.
> The syntax is: `<name><operator><value>` with `<operator>` among `=`, `==`, `>`, `>=`, `<`, `<=` <br>
> The `<operator><value>` can be ommitted to filter on the existence of a named argument. <br>
> Ex: "user==georges", "id=315", "weight<=50.0", "weight"

The returned value is a chronologic list of NamedTuples. The `sslogread.Log` structure is:
| Parameter name   | Type             | Description                                              |
|------------------|------------------|----------------------------------------------------------|
| `level`          | `string`         | Log level                                                |
| `timestampUtcNs` | `integer`        | Log timestamp in nanosecond since UTC epoch              |
| `category`       | `string`         | Log category                                             |
| `thread`         | `string`         | Log thread name                                          |
| `format`         | `string`         | Log format string already filled with arguments          |
| `arguments`      | `list of tuples` | List of log arguments as tuple (name, unit, typed value) |
| `buffer`         | `string`         | Log buffer in base64                                     |

### Examples

Display all the logs (no filtering) in text:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query()
for log in result:
   print("%d) %s" % (log['timestampUtcNs'], log['format']))
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Keep only the logs with a binary buffer and on a category matching `*engine*`:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query( { 'buffer_size_min': 1, 'category':'*engine*' } )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs on a category matching `*engine*` but not on a thread matching `worker*`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query( { 'no_thread': "worker*", 'category':'*engine*' } )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs which possess an argument named `id` and another named `user`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query( { 'arguments': ["id", "user"] } )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
> [!WARNING]
> The argument name shall be exact, no pattern matching for argument's names and values

Select the logs which possess an argument named `user` which has a value `github_agent`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query( { 'arguments': ["user==github_agent"] } ) # Note: '=' or '==' are identical
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs on category `transaction` and which possess an argument named `id` which has a value higher than 356 and lower or equal to 1000
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query( { 'category': "transaction", 'arguments': ["id>356", "id<=1000"] } )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs with level >= error, with category name containing 'car', which contain both an argument named 'color' and one named 'wheels' with value >= 4
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query( { 'level_min':'error', 'category':'*car*', 'arguments':['color', 'wheels>=4'] } )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs on category `transaction` and which possess an argument named `id` which has a value higher than 356 and lower or equal to 1000 <br>
OR the logs with level >= error, with category name containing 'car', which contain both an argument named 'color' and one named 'wheels' with value >= 4
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query( { 'category': "transaction", 'arguments': ["id>356", "id<=1000"] },
                        { 'level_min':'error', 'category':'*car*', 'arguments':['color', 'wheels>=4'] } )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## sslogread.LogSession.get_strings

This method returns a list of strings matching both a content pattern and a selection of string flags. <br>
It is basically a query on strings.

### Declaration

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
def LogSession.get_strings(pattern="", in_category=False, in_thread=False, in_format=False, in_arg_name=False, in_arg_value=False, in_arg_unit=False)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

> [!TIP]
> If the pattern is empty, then all strings are accepted for this criteria. <br>
> If no flag on the string type is set, then all strings are accepted for these criteria.

### Example

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
// Get strings used as a category, matching the pattern "/scheduler/*/worker"
strings = session.get_strings(pattern="/scheduler/*/worker", in_category=True)

// Get all thread name strings
strings = session.get_strings(in_thread=True)

// Get all format strings or argument value strings, containing the word "User"
strings = session.get_strings(pattern="*User*", in_format=True, in_arg_value=True)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## License

Released under the MIT license.
