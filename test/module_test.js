var test = require("test");
test.setup();

var fs = require('fs');
var path = require('path');
var a, b;

const bin_path = path.dirname(process.execPath);

describe("module", () => {
    after(() => {
        try {
            fs.unlink(path.join(__dirname, 'module', 'p6_1'));
        } catch (e) { }
    });

    it("native module toJSON", () => {
        JSON.stringify(require("os"));
    });

    it("resolve", () => {
        assert.equal(require.resolve('./module/a'),
            path.join(__dirname, 'module', 'a.js'));
        assert.equal(require.resolve('./module/test.zip$/folder/b'),
            path.join(__dirname, 'module', 'test.zip$', 'folder', 'b.js'));
        assert.equal(require.resolve('node_mod1'),
            path.join(__dirname, 'node_modules', 'node_mod1.js'));

        assert.equal(require.resolve('./module/d1'),
            path.join(__dirname, 'module', 'd1'));
    });

    it("circular dependency", () => {
        a = require('./module/a1');
        b = require('./module/b1');

        assert.property(a, "a", 'a exists');
        assert.property(b, "b", 'b exists')
        assert.strictEqual(a.a().b, b.b, 'a gets b');
        assert.strictEqual(b.b().a, a.a, 'b gets a');
    });

    it("exports", () => {
        var a = require('./module/a2');
        var foo = a.foo;
        assert.strictEqual(a.foo(), a, 'calling a module member');
        a.set(10);
        assert.strictEqual(a.get(), 10, 'get and set')
    });

    it("require error", () => {
        assert.throws(() => {
            require('./bogus');
        });
    });

    it("require compile error", () => {
        assert.throws(() => {
            require('./module/require_bug');
        });
    });

    it("share require", () => {
        a = require('./module/a3');
        b = require('./b');
        assert.strictEqual(a.foo, b.foo,
            'a and b share foo through a relative require');
    });

    it("transitive", () => {
        assert.strictEqual(require('./module/a4').foo(), 1, 'transitive');
    });

    it("require json", () => {
        assert.deepEqual(require('./module/data'), {
            "a": 100,
            "b": 200
        });
    });

    it("require error json", () => {
        assert.throws(() => {
            require('./module/data_err');
        });
    });

    it("require .js module folder", () => {
        assert.deepEqual(require('./module/p4.js'), {
            "v": 100
        });
    });

    it("require ./..", () => {
        assert.equal(require('./module/p7/t1').p7, require('./module/p7'));
        assert.equal(require('./module/p7/t.js').p7, require('./module/p7'));
    });

    it("require no-ext file", () => {
        assert.deepEqual(require('./module/d1'), {
            "d": 100
        });
    });

    xit("require async file", () => {
        assert.equal(require('./module/d2'), 100);
    });

    it("support symlink", () => {
        fs.symlink(path.join(__dirname, 'module', 'p6'), path.join(__dirname, 'module', 'p6_1'));
        assert.equal(require('./module/p6/t'), require('./module/p6_1/t'));
    });

    describe("package.json", () => {
        it("main", () => {
            var a = require('./module/p1');
            assert.deepEqual(a, {
                "a": 100
            });

            assert.equal(a, require('./module/p1'));
            assert.equal(a, require('./module/p1/main'));
        });

        describe("exports", () => {
            it("simple", () => {
                var a = require('./module/p1.1');
                assert.deepEqual(a, {
                    "a": 101
                });

                assert.equal(a, require('./module/p1.1'));
                assert.equal(a, require('./module/p1.1/main'));
            });

            it(".", () => {
                var a = require('./module/p1.2');
                assert.deepEqual(a, {
                    "a": 102
                });

                assert.equal(a, require('./module/p1.2'));
                assert.equal(a, require('./module/p1.2/main'));
            });

            it("require", () => {
                var a = require('./module/p1.3');
                assert.deepEqual(a, {
                    "a": 103
                });

                assert.equal(a, require('./module/p1.3'));
                assert.equal(a, require('./module/p1.3/main'));
            });

            it("default", () => {
                var a = require('./module/p1.4');
                assert.deepEqual(a, {
                    "a": 104
                });

                assert.equal(a, require('./module/p1.4'));
                assert.equal(a, require('./module/p1.4/main'));
            });

            it("./require", () => {
                var a = require('./module/p1.5');
                assert.deepEqual(a, {
                    "a": 105
                });

                assert.equal(a, require('./module/p1.5'));
                assert.equal(a, require('./module/p1.5/main'));
            });

            it("./require/default", () => {
                var a = require('./module/p1.6');
                assert.deepEqual(a, {
                    "a": 106
                });

                assert.equal(a, require('./module/p1.6'));
                assert.equal(a, require('./module/p1.6/main'));
            });

            it("./default/require", () => {
                var a = require('./module/p1.7');
                assert.deepEqual(a, {
                    "a": 107
                });

                assert.equal(a, require('./module/p1.7'));
                assert.equal(a, require('./module/p1.7/main'));
            });

            it("./node/require", () => {
                var a = require('./module/p1.8');
                assert.deepEqual(a, {
                    "a": 108
                });

                assert.equal(a, require('./module/p1.8'));
                assert.equal(a, require('./module/p1.8/main'));
            });

            it("./require|default", () => {
                var a = require('./module/p1.9');
                assert.deepEqual(a, {
                    "a": 109
                });

                assert.equal(a, require('./module/p1.9'));
                assert.equal(a, require('./module/p1.9/main'));
            });
        });

        it("default entry", () => {
            var a = require('./module/p2');
            assert.deepEqual(a, {
                "a": 200
            });

            assert.equal(a, require('./module/p2'));
            assert.equal(a, require('./module/p2/index'));
        });

        it("no json", () => {
            var a = require('./module/p3');
            assert.deepEqual(a, {
                "a": 300
            });

            assert.equal(a, require('./module/p3'));
            assert.equal(a, require('./module/p3/index'));
        });

        it("entry is folder", () => {
            var a = require('./module/p5');
            assert.deepEqual(a, {
                "a": 500
            });

            assert.equal(a, require('./module/p5'));
            assert.equal(a, require('./module/p5/lib'));
            assert.equal(a, require('./module/p5/lib/index'));
        });
    });

    describe("node_modules", () => {
        it("root folder", () => {
            var a = require('node_mod1');
            assert.deepEqual(a, {
                "a": 100
            });

            assert.equal(a, require('./node_modules/node_mod1'));
        });

        it("current folder", () => {
            var a = require('./module/mod_test').require("node_mod2");
            assert.deepEqual(a, {
                "a": 200
            });

            assert.equal(a, require('./module/node_modules/node_mod2'));
        });

        it("parent folder", () => {
            var a = require('./module/mod_test').require("node_mod4");
            assert.deepEqual(a, {
                "a": 400
            });

            assert.equal(a, require('./node_modules/node_mod4'));
        });

        it("priority", () => {
            var a = require('./module/mod_test').require("node_mod3");
            assert.deepEqual(a, {
                "a": 300
            });

            assert.equal(a, require('./module/node_modules/node_mod3'));
        });
    });

    it("zip virtual path", () => {
        assert.deepEqual(require('./module/test.src/folder/b.js'),
            require('./module/test.zip$/folder/b.js'));

        assert.equal(require('./module/p4.zip$').a, 100);
        assert.equal(require('./module/p4').a, 100);
    });

    it("strack", () => {
        assert.ok(require("./module/stack").func().match(/module_test/));
    });

    it("support exports in script", () => {
        run('./module/exec18');
    });

    it("support embed script module", () => {
        var s = require('stream');
        var s1 = require('stream.js');
        assert.equal(s, s1);
    });

    it("addon module", () => {
        var m = require(path.join(bin_path, '1_hello_world'));
        assert.equal(m.hello(), "world");
    });
});

require.main === module && test.run(console.DEBUG);