import { expectType } from 'tsd';

import * as plutoprint from './index'

const book = new plutoprint.Book();

expectType<plutoprint.Book>(book);

expectType<number>(book.pageCount);
expectType<number>(book.documentWidth);
expectType<number>(book.documentHeight);
expectType<number>(book.viewportWidth);
expectType<number>(book.viewportHeight);

expectType<plutoprint.Book>(book.loadHtml('<h1>Hello World</h1>'));

expectType<void>(book.writeToPdf('hello.pdf'))
expectType<Buffer>(book.writeToPdfBuffer())

expectType<void>(book.writeToPng('hello.png'))
expectType<Buffer>(book.writeToPngBuffer())

expectType<plutoprint.Book>(plutoprint.createBook());

expectType<string>(plutoprint.plutobookVersion);
expectType<string>(plutoprint.plutobookBuildInfo);

expectType<number>(plutoprint.MIN_PAGE_COUNT);
expectType<number>(plutoprint.MAX_PAGE_COUNT);

expectType<number>(plutoprint.UNITS_PT);
expectType<number>(plutoprint.UNITS_PC);
expectType<number>(plutoprint.UNITS_IN);
expectType<number>(plutoprint.UNITS_CM);
expectType<number>(plutoprint.UNITS_MM);
expectType<number>(plutoprint.UNITS_PX);
