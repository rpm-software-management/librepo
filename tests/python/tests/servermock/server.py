from flask import Flask
from yum_mock.yum_mock import yum_mock

app = Flask(__name__)
#app.register_blueprint(working_repo)
app.register_blueprint(yum_mock, url_prefix='/yum')


if __name__ == '__main__':
    app.run(debug=False, threaded=True)
