#!/usr/bin/env seed

Gtk = imports.gi.Gtk;
Polkit = imports.gi.Polkit;
PolkitGtk = imports.gi.PolkitGtk;

Gtk.init (Seed.argv);

var vbox = new Gtk.VBox ();

var label = new Gtk.Label ({label: "Hello world"});
label.set_alignment (0 ,0);
vbox.pack_start (label, false, true, 1);

var lock_button = new PolkitGtk.LockButton ({action_id: "org.freedesktop.devicekit.disks.filesystem-mount"});
vbox.pack_start (lock_button, false, true, 1);

var window = new Gtk.Window ({title: "PolkitGtk Seed Example", resizable: false});
window.resize (250, 250);
window.signal.hide.connect (Gtk.main_quit);
window.add (vbox);
window.show_all ();

var authority = Polkit.Authority.get ();
authority.signal.changed.connect (function () {print ("authority changed!");});

Gtk.main ();

