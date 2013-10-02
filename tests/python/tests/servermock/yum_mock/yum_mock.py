from flask import Blueprint, render_template, abort, send_file, request, Response
from functools import wraps
import os
from .config import AUTH_USER, AUTH_PASS

yum_mock = Blueprint('yum_mock', __name__,
                        template_folder='templates',
                        static_folder='static')

@yum_mock.route('/harm_checksum/<keyword>/<path:path>')
def harm_checksum(keyword, path):
    """Append two newlines to content of a file (from the static dir) with
    specified keyword in the filename. If the filename doesn't contain
    the keyword, content of the file is returnen unchanged."""

    if "static/" not in path:
        # Support changing only files from static directory
        abort(400)
    path = path[path.find("static/"):]

    try:
        with yum_mock.open_resource(path) as f:
            data = f.read()
            if keyword in os.path.basename(path):
                return "%s\n\n" %data
            return data
    except IOError:
        # File probably doesn't exist or we can't read it
        abort(404)

@yum_mock.route("/not_found/<keyword>/<path:path>")
def not_found(keyword, path):
    """For each file containing keyword in the filename, http status
    code 404 will be returned"""

    if "static/" not in path:
        abort(400)
    path = path[path.find("static/"):]

    try:
        with yum_mock.open_resource(path) as f:
            data = f.read()
            if keyword in os.path.basename(path):
                abort(404)
            return data
    except IOError:
        # File probably doesn't exist or we can't read it
        abort(404)

@yum_mock.route("/badurl/<path:path>")
def badurl(path):
    """Just return 404 for each url with this prefix"""
    abort(404)

@yum_mock.route("/badgpg/<path:path>")
def badgpg(path):
    """Instead of <path>/repomd.xml.asc returns
    content of <path>/repomd.xml.asc.bad"""
    if "static/" not in path:
        abort(400)
    path = path[path.find("static/"):]
    if path.endswith("repomd.xml.asc"):
        path = path + ".bad"

    try:
        with yum_mock.open_resource(path) as f:
            return f.read()
    except IOError:
        # File probably doesn't exist or we can't read it
        abort(404)

# Basic Auth

def check_auth(username, password):
    """This function is called to check if a username /
    password combination is valid.
    """
    return username == AUTH_USER and password == AUTH_PASS

def authenticate():
    """Sends a 401 response that enables basic auth"""
    return Response(
    'Could not verify your access level for that URL.\n'
    'You have to login with proper credentials', 401,
    {'WWW-Authenticate': 'Basic realm="Login Required"'})

def requires_auth(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        auth = request.authorization
        if not auth or not check_auth(auth.username, auth.password):
            return authenticate()
        return f(*args, **kwargs)
    return decorated

@yum_mock.route("/auth_basic/<path:path>")
@requires_auth
def secret_repo_basic_auth(path):
    """Page secured with basic HTTP auth
    User: admin Password: secret"""
    if "static/" not in path:
        abort(400)
    path = path[path.find("static/"):]

    try:
        with yum_mock.open_resource(path) as f:
            data = f.read()
            return data
    except IOError:
        abort(404)

