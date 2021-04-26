from http.server import BaseHTTPRequestHandler, HTTPServer
from optparse import OptionParser
try:
    from yum_mock.yum_mock import yum_mock_handler
except (ValueError, ImportError):
    from .yum_mock.yum_mock import yum_mock_handler


def start_server(port, host="127.0.0.1", handler=None):
    if handler is None:
        handler = yum_mock_handler(port)
    with HTTPServer((host, port), handler) as server:
        server.serve_forever()

if __name__ == '__main__':
    parser = OptionParser("%prog [options]")
    parser.add_option(
        "-p", "--port",
        default=5000,
        type="int",
    )
    parser.add_option(
        "-n", "--host",
        default="127.0.0.1",
    )
    options, args = parser.parse_args()

    start_server(options.port, options.host)

