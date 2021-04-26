import base64
from http.server import BaseHTTPRequestHandler, HTTPServer
import os
import sys

from .config import AUTH_USER, AUTH_PASS


def file_path(path):
    return(os.path.join(os.path.dirname(os.path.abspath(__file__)), path))


def yum_mock_handler(port):

    class YumMockHandler(BaseHTTPRequestHandler):
        _port = port

        def return_bad_request(self):
            self.send_response(400)
            self.end_headers()

        def return_not_found(self):
            self.send_response(404)
            self.end_headers()

        def return_ok_with_message(self, message, content_type='text/html'):
            if content_type == 'text/html':
                message = bytes(message, 'utf8')
            self.send_response(200)
            self.send_header('Content-type', content_type)
            self.send_header('Content-Length', str(len(message)))
            self.end_headers()
            self.wfile.write(message)

        def parse_path(self, test_prefix='', keyword_expected=False):
            path = self.path[len(test_prefix):]
            if keyword_expected:
                keyword, path = path.split('/', 1)
            # Strip arguments
            if '?' in path:
                path = path[:path.find('?')]
            if keyword_expected:
                return keyword, path
            return path

        def serve_file(self, path, harm_keyword=None):
            if "static/" not in path:
                # Support changing only files from static directory
                return self.return_bad_request()
            path = path[path.find("static/"):]
            try:
                with open(file_path(path), 'rb') as f:
                    data = f.read()
                    if harm_keyword is not None and harm_keyword in os.path.basename(file_path(path)):
                        data += b"\n\n"
                    return self.return_ok_with_message(data, 'application/octet-stream')
            except IOError:
                # File probably doesn't exist or we can't read it
                return self.return_not_found()

        def authenticate(self):
            """Sends a 401 response that enables basic auth"""
            self.send_response(401)
            self.send_header('Content-type', 'text/html')
            self.send_header('WWW-Authenticate', 'Basic realm="Login Required')
            self.end_headers()
            message = (
                'Could not verify your access level for that URL.\n'
                'You have to login with proper credentials'
            )
            self.wfile.write(bytes(message, "utf8"))

        def check_auth(self):
            if self.headers.get('Authorization') is None:
                return False
            expected_authorization = 'Basic {}'.format(
                base64.b64encode('{}:{}'.format(AUTH_USER, AUTH_PASS).encode()).decode()
            )
            if self.headers.get('Authorization') != expected_authorization:
                return False
            return True

        def serve_mirrorlist_or_metalink_with_right_port(self):
            path = self.parse_path()
            if "static/" not in path:
                return self.return_bad_request()
            path = path[path.find("static/"):]
            try:
                with open(file_path(path), 'r') as f:
                    data = f.read()
                    data = data.replace(":{PORT_PLACEHOLDER}", ":%d" % self._port)
                    return self.return_ok_with_message(data)
            except IOError:
                # File probably doesn't exist or we can't read it
                return self.return_not_found()

        def serve_harm_checksum(self):
            """Append two newlines to content of a file (from the static dir) with
            specified keyword in the filename. If the filename doesn't contain
            the keyword, content of the file is returnen unchanged."""
            keyword, path = self.parse_path('/yum/harm_checksum/', keyword_expected=True)
            self.serve_file(path, harm_keyword=keyword)

        def serve_not_found(self):
            """For each file containing keyword in the filename, http status
            code 404 will be returned"""
            keyword, path = self.parse_path('/yum/not_found/', keyword_expected=True)
            if keyword in os.path.basename(file_path(path)):
                return self.return_not_found()
            self.serve_file(path)

        def serve_badurl(self):
            """Just return 404 for each url with this prefix"""
            return self.return_not_found()

        def serve_badgpg(self):
            """Instead of <path>/repomd.xml.asc returns content of <path>/repomd.xml.asc.bad"""
            path = self.parse_path('/yum/badgpg/')
            if path.endswith("repomd.xml.asc"):
                path += ".bad"
            self.serve_file(path)

        def serve_auth_basic(self):
            """Page secured with basic HTTP auth; User: admin Password: secret"""
            if not self.check_auth():
                return self.authenticate()
            path = self.parse_path('/yum/auth_basic/')
            self.serve_file(path)

        def serve_static(self):
            path = self.parse_path()
            self.serve_file(path)

        def do_GET(self):
            if self.path.startswith('/yum/static/mirrorlist/'):
                return self.serve_mirrorlist_or_metalink_with_right_port()
            if self.path.startswith('/yum/static/metalink/'):
                return self.serve_mirrorlist_or_metalink_with_right_port()
            if self.path.startswith('/yum/harm_checksum/'):
                return self.serve_harm_checksum()
            if self.path.startswith('/yum/not_found/'):
                return self.serve_not_found()
            if self.path.startswith('/badurl/'):
                return self.serve_badurl()
            if self.path.startswith('/yum/badgpg/'):
                return self.serve_badgpg()
            if self.path.startswith('/yum/auth_basic/'):
                return self.serve_auth_basic()
            return self.serve_static()

    return YumMockHandler

