mr_passthru.c - registers OIDs for Microsoft LDAP_MATCHING_RULE_IN_CHAIN


Introduction
============

In Microsoft Active Directory, groups can be organised in a hierarchical manner, eg one group can contain other groups. Often, users, roles and applications are defined as

    user --> global-group --> local-group --> application

It means a user is member of a global-group (considered as role), this global- group is member of a local-group, which is used to allow access to an application. This is generally considered as best practice by Microsoft.

When using LDAP against Microsoft AD to retrieve the groups of a user, the only group listed will be those where he has direct membership (global-groups). As a consequence, getting the group chain needs more than one LDAP query, and the application starts to use recursive queries, which heavily load the AD servers.

Microsoft AD provides an implementation of RFC 2254 (The String Representation of LDAP Search Filters) where they use OID’s to provide LDAP access to internal AD search functions and options, including recursive behaviour.

<hr>

Taken from Microsoft documentation:

The LDAP_MATCHING_RULE_IN_CHAIN is a matching rule OID that is designed to provide a method to look up the ancestry of an object. Many applications using AD and AD LDS usually work with hierarchical data, which is ordered by parent-child relationships. Previously, applications performed transitive group expansion to figure out group membership, which used too much network bandwidth; applications needed to make multiple roundtrips to figure out if an object fell "in the chain" if a link is traversed through to the end. An example of such a query is one designed to check if a user "user1" is a member of group "group1". You would set the base to the user DN (cn=user1, cn=users, dc=x) and the scope to base, and use the following query.

    (memberof:1.2.840.113556.1.4.1941:=cn=Group1,OU=groupsOU,DC=x)

Similarly, to find all the groups that "user1" is a member of, set the base to the groups container DN; for example (OU=groupsOU, dc=x) and the scope to subtree, and use the following filter.

    (member:1.2.840.113556.1.4.1941:=(cn=user1,cn=users,DC=x))

Note that when using LDAP_MATCHING_RULE_IN_CHAIN, scope is not limited - it can be base, one-level, or subtree. Some such queries on subtrees may be more processor intensive, such as chasing links with a high fan-out; that is, listing all the groups that a user is a member of. Inefficient searches will log appropriate event log messages, as with any other type of query.

Reference : http://msdn.microsoft.com/en-us/library/aa746475(v=vs.85).aspx

--------------------------------------------------------------------------------

Notes:

- In some Microsoft documents, the OID is named LDAP_MATCHING_RULE_TRANSITIVE_EVAL.
- Microsoft provides two other OIDs to perform bitwise comparisons of numeric values. They are much less useful in the context of a LDAP proxy so they have been omitted from the implementation of the module below.

Description of the problem
==========================

When used as a LDAP proxy with back_meta, the filter form above will not pass through the proxy, because the OID 1.2.840.113556.1.4.1941 is unknown within OpenLDAP, and as the manual page of slapd-meta states:

--------------------------------------------------------------------------------

The proxy instance of slapd(8) must contain schema information for the attributes and objectClasses used in filters, request DN and request-related data in general. It should also contain schema information for the data returned by the proxied server. It is the responsibility of the proxy administrator to keep the schema of the proxy lined up with that of the proxied server.

--------------------------------------------------------------------------------

The symptom, when searching the debug log, is an undefined filter:

    ...
    538dd110 begin get_filter
    538dd110 EXTENSIBLE
    ...
    538dd110 end get_filter 0
    538dd110     filter: (?=undefined)
    ...


The solution
============

Pierangelo Masarati has written a piece of C code (slapd module) that registers OIDs for the LDAP_MATCHING_RULE_IN_CHAIN matching rule. This module has to be loaded after the memberof module, which declares the "memberOf" attribute (otherwise unknown to OpenLDAP's slapd).

Compilation and installation:

    cd contrib/slapd-modules/mr_passthru
    make
    adapt the "prefix" in Makefile
    make install

Configuration:

The configuration within slapd.conf would only add those two lines to accept the new OID:

    moduleload    contrib-memberof.so
    moduleload    contrib-mr_passthru.so


ACKNOWLEDGEMENTS
================

Thanks to Pierangelo Masarati for the mr_passthru.c code and explanations.

Charles Bueche <bueche@netnea.com>, 6.6.2014
