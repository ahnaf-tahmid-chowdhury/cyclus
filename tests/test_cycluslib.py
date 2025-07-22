"""Tests for cyclus wrappers"""
import os
import subprocess
from functools import wraps

from cyclus import lib

from tools import dbtest

@dbtest
def test_name(db, fname, backend):
    obs = db.name
    assert fname ==  obs


@dbtest
def test_simid(db, fname, backend):
    df = db.query("AgentEntry")
    simid = df['SimId']
    exp = simid[0]
    for obs in simid:
        assert exp ==  obs

@dbtest
def test_conds_ae(db, fname, backend):
    obs = db.query("AgentEntry", [('Kind', '==', 'Region')])
    assert 1 ==  len(obs)
    assert 'Region' ==  obs['Kind'][0]
    assert ':agents:NullRegion' ==  obs['Spec'][0]


@dbtest
def test_conds_comp(db, fname, backend):
    conds = [('NucId', '==', 922350000), ('MassFrac', '<=', 0.0072)]
    df = db.query("Compositions", conds)
    assert (0 <  len(df))
    for row in df['MassFrac']:
        assert (row <  0.00720000001)


@dbtest
def test_dbopen(db, fname, backend):
    db = lib.dbopen(fname)

@dbtest
def test_schema(db, fname, backend):
    schema = db.schema("AgentEntry")
    assert 8 ==  len(schema)
    cols = ["SimId", "AgentId", "Kind", "Spec", "Prototype", "ParentId", "Lifetime", "EnterTime"]
    dbs = [7, 1, 5, 5, 5, 1, 1, 1]
    for i, ci in enumerate(schema):
        assert "AgentEntry" ==  ci.table
        assert cols[i] ==  ci.col
        assert dbs[i] ==  ci.dbtype
        assert i ==  ci.index
        assert 1 ==  len(ci.shape)
        assert -1 ==  ci.shape


def test_position():
    p1 = lib.Position(42.65, 28.6)
    p2 = lib.Position(42.65, 28.6)
    d = p1.distance(p2)
    assert 0.0 ==  d



