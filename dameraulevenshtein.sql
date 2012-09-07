--
-- Copyright (c) 2008 Meding Software Technik - All Rights Reserved.
--
-- Damerau/Levenshtein calculator (MySQL UDF)
--
-- 1. Register the UDF
-- 2. Run a couple of quick tests
--

DROP FUNCTION IF EXISTS dameraulevenshtein;
CREATE FUNCTION dameraulevenshtein RETURNS INTEGER SONAME 'libdameraulevenshtein.so';

--
-- Tests:
--

-- returns 0
SELECT 'D/L distance ''laber''/''laber'' => 0',dameraulevenshtein('laber','laber') calculated;

-- returns 3
SELECT 'D/L distance ''laber''/''lall'' => 3',dameraulevenshtein('laber','lall') calculated;

