from flask import Flask
from optparse import OptionParser
try:
    from yum_mock.yum_mock import yum_mock
except (ValueError, ImportError):
    from .yum_mock.yum_mock import yum_mock

app = Flask(__name__)
#app.register_blueprint(working_repo)
app.register_blueprint(yum_mock, url_prefix='/yum')


if __name__ == '__main__':
    parser = OptionParser("%prog [options]")
    parser.add_option(
        "-d", "--debug",
        action="store_true",
    )
    parser.add_option(
        "-p", "--port",
        default=5000,
        type="int",
    )
    parser.add_option(
        "-n", "--host",
        default="127.0.0.1",
    )
    parser.add_option(
        "--passthrough_errors",
        action="store_true",
    )
    options, args = parser.parse_args()

    kwargs = {
        "threaded": True,
        "debug": options.debug,
        "port": options.port,
        "host": options.host,
        "passthrough_errors": options.passthrough_errors,
    }

    app.run(**kwargs)
