Introduction

We are using Flask as the Backend Framework.

Start:

pip install -r requirements.txt

To generate the database, open a python terminal and do the following:

from app import db # The main app file.

db.create_all()

This will create the database with the tables matching the models.

This is a test.
